#pragma once

#include <atomic>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <list>
#include <tuple>
#include <exception>
#include <boost/chrono.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include "AsyncTaskGroup.h"
#include "DateTime.h"
#include "StringConversion.h"
#include "TypeName.h"
#include "TypeTraits.h"

namespace Fmi
{
    enum class PoolInitType
    {
        Sequential,
        Parallel
    };

    /**
     * @brief A thread-safe object pool implementation
     *
     * This class implements a thread-safe object pool for managing reusable objects.
     * It allows pre-allocation of a specified number of objects and can grow
     * dynamically up to a maximum limit.
     *
     * The pool can be initialized either sequentially or in parallel, depending on the
     * parameters from Args are passed to the ItemType constructor when creating new items.
     *
     * All constructor arguments must be copy constructable as they are stored in the pool instance.
     * Note that if reference of type is provided, argument itself must be copyable as well
     * (as references are removed when storing in tuple).
     *
     * The pool items and all synchronization primitives live in a shared state object that is
     * kept alive by every outstanding Pool<>::Ptr. As a result items acquired from the pool
     * remain valid even if the pool object itself is destroyed before the items are released.
     *
     * Template parameters:
     * - InitType: Specifies the initialization type (sequential or parallel).
     * - ItemType: The type of objects managed by the pool.
     * - Args: Additional arguments passed to the ItemType constructor or factory method when creating new items.
     */
    template <PoolInitType InitType, typename ItemType, typename... Args>
    class Pool
    {
        static_assert(are_all_parameters_copyable<typename std::decay<Args>::type...>(),
            "All Args parameters must be copyable");

        struct ItemRec
        {
            /**
             * @brief Actual pool item
             */
            std::unique_ptr<ItemType> data;

            /**
             * @brief Pointer to the next free item record in the pool.
             *
             * Value is nullptr when one of following is true:
             * - Item is currently in use
             * - Item is the last item in the free item chain
             */
            ItemRec* next = nullptr;

            /**
            *  @brief Indicates whether the item is currently in use
            *
            *  nextptr is not sufficient as it is nullptr also for the last free item
            */
            bool in_use = false;

            ItemRec(std::unique_ptr<ItemType>&& item)
                : data(std::move(item))
            {
            }

            ItemRec(const ItemRec&) = delete;
            ItemRec& operator=(const ItemRec&) = delete;
            ItemRec(ItemRec&&) = default;
            ItemRec& operator=(ItemRec&&) = default;
        };

        /**
         * @brief State shared between the pool object and the deleters of all handed out items
         *
         * Held through std::shared_ptr both by the pool and by every Pool<>::Ptr deleter, so it
         * outlives the pool object while items are still in use. All mutable pool state is
         * protected by the mutex contained here. This avoids any access to destroyed memory
         * when items are released after (or concurrently with) pool destruction.
         */
        struct SharedState
        {
            mutable boost::mutex mutex;
            boost::condition_variable cond_var;

            /**
             * @brief False once the pool object destructor has run
             */
            bool alive = true;

            /**
             * @brief Current number of items in the pool
             */
            std::size_t current_size = 0;

            /**
             * @brief Next size of the pool when growing (used to control max_size limit)
             *
             * Intended to avoid growing over max_size in case of concurrent calls to acquire()
             */
            std::size_t next_current_size = 0;
            std::size_t in_use_count = 0;

            ItemRec* top = nullptr;
            std::list<ItemRec> pool_data;
        };

    public:
        class Ptr : private std::unique_ptr<ItemType, std::function<void(ItemType*)>>
        {
            friend class Pool;
            using base = std::unique_ptr<ItemType, std::function<void(ItemType*)>>;

            Ptr(ItemType* ptr, std::function<void(ItemType*)> release)
                : base(ptr, std::move(release))
            {
            }

        public:
            Ptr(Ptr&& other) = default;

            // We do not want to expose all std::unique_ptr<> methods for example release()
            inline operator ItemType*() const { return this->base::get(); }
            inline operator bool() const { return this->base::get() != nullptr; }
            inline ItemType* operator->() const { return this->base::get(); }
            inline ItemType* get() const { return this->base::get(); }
            inline void reset() { this->base::reset(); }
        };

        Pool(const Pool&) = delete;
        Pool& operator=(const Pool&) = delete;
        // Moving would leave the source pool without shared state: not supported
        Pool(Pool&&) = delete;
        Pool& operator=(Pool&&) = delete;

        /**
         * @brief Constructs a Pool with specified start and maximum sizes
         *
         * @param start_size The initial number of items to pre-allocate in the pool (minimum 2)
         * @param max_size The maximum number of items the pool can grow to (must be >= start_size)
         * @param args Additional arguments passed to the ItemType constructor when creating new items
         *
         * Note that if max_size is larger than start_size, related objects must exist during the whole pool lifetime
         * in case when they are passed to constructor as references or pointers.
         */
        Pool(std::size_t start_size, std::size_t max_size, Args... args)
        try
            : start_size(std::max(std::size_t(2), start_size))
            , max_size(std::max(start_size, max_size))
            , constructor_args(typename std::decay<Args>::type(args)...)
            , createItemCb([this]()
              { return std::make_unique<ItemType>(std::get<typename std::decay<Args>::type>(constructor_args)...); })
            , state(std::make_shared<SharedState>())
        {
            init(args...);
        }
        catch (...)
        {
            throw Fmi::Exception::Trace(BCP, "Error initializing Pool of type " +
                Fmi::demangle_cpp_type_name(typeid(ItemType).name()));
        }

        /**
         * @brief Constructs a Pool with specified start and maximum sizes and a custom item creation callback
         *
         * @param createItemCb A callback function that creates and returns a unique_ptr to a new ItemType instance
         * @param start_size The initial number of items to pre-allocate in the pool (minimum 2)
         * @param max_size The maximum number of items the pool can grow to (must be >= start_size)
         * @param args Additional arguments passed to the createItemCb when creating new items
         *
         * Note that if max_size is larger than start_size, related objects must exist during the whole pool lifetime
         * in case when they are passed to factory method as references or pointers.
         */
        Pool(
            std::function<std::unique_ptr<ItemType>(Args... args)> createItemCb_,
            std::size_t start_size,
            std::size_t max_size,
            Args... args)
        try
            : start_size(std::max(std::size_t(2), start_size))
            , max_size(std::max(start_size, max_size))
            , constructor_args(typename std::decay<Args>::type(args)...)
            , createItemCb([this, createItemCb_]()
              { return createItemCb_(std::get<typename std::decay<Args>::type>(constructor_args)...); })
            , state(std::make_shared<SharedState>())
        {
            init(args...);
        }
        catch (...)
        {
            throw Fmi::Exception::Trace(BCP, "Error initializing Pool of type " +
                Fmi::demangle_cpp_type_name(typeid(ItemType).name()));
        }

        virtual ~Pool()
        {
            std::size_t items_in_use = 0;
            std::size_t total_size = 0;

            {
                boost::unique_lock<boost::mutex> lock(state->mutex);
                state->alive = false;
                items_in_use = state->in_use_count;
                total_size = state->current_size;
                // Wake up all threads blocked in acquire() so that they can observe the shutdown
                state->cond_var.notify_all();
            }

            if (items_in_use)
            {
                // There are some items in use. They remain valid: the shared state (and thus the
                // items) is kept alive by the Ptr deleters and released when the last Ptr is gone.
                std::cerr << "Pool of " << Fmi::demangle_cpp_type_name(typeid(ItemType).name())
                          << " is being destroyed while items are still in use" << std::endl;
                std::cerr << "Items in use: " << items_in_use << std::endl;
                std::cerr << "Total pool size: " << total_size << std::endl;
            }
        }

        Ptr get()
        {
            ItemRec* rec = acquire(std::nullopt);
            return Ptr(
                rec->data.get(),
                [state = this->state, rec](ItemType*) { releaseItem(*state, rec); });
        }

        Ptr get(const Fmi::TimeDuration& timeout)
        {
            ItemRec* rec = acquire(timeout);
            return Ptr(
                rec->data.get(),
                [state = this->state, rec](ItemType*) { releaseItem(*state, rec); });
        }

        std::size_t size() const
        {
            boost::unique_lock<boost::mutex> lock(state->mutex);
            return state->current_size;
        }

        std::size_t in_use() const
        {
            boost::unique_lock<boost::mutex> lock(state->mutex);
            return state->in_use_count;
        }

        void dumpInfo(std::ostream& os)
        {
            int count = 0;
            boost::unique_lock<boost::mutex> lock(state->mutex);
            os << "Pool info for items of type " << Fmi::demangle_cpp_type_name(typeid(ItemType).name()) << std::endl;
            os << "Total items: " << state->pool_data.size() << std::endl;
            os << "In use items: " << state->in_use_count << std::endl;
            os << "Top free item: " << (void*)state->top << std::endl;
            for (const auto& item : state->pool_data)
            {
                os << "Item[" << ++count << "]: " << (void*)&item
                   << ", next: " << (void*)item.next << std::endl;
            }
        }

    private:

        void init(Args... args)
        {
            const auto grow = [this]() {
                std::unique_ptr<ItemType> new_item(createItemCb());
                boost::unique_lock<boost::mutex> lock(state->mutex);
                // Add new item to the pool. List iterators are not invalidated by growing list
                ItemRec& item_rec = state->pool_data.emplace_back(ItemRec(std::move(new_item)));
                item_rec.next = state->top;
                state->top = &item_rec;
                state->current_size++;
                state->next_current_size++;
            };

            if constexpr (InitType == PoolInitType::Sequential)
            {
                for (std::size_t i = 0; i < start_size; ++i)
                {
                    grow();
                }
            }
            else if constexpr (InitType == PoolInitType::Parallel)
            {
                std::atomic<bool> have_errors = false;
                std::atomic<int> num_errors = 0;

                const auto init_task_error = [this, &have_errors, &num_errors]
                    (const std::string& name)
                {
                    bool trueVal = true;
                    num_errors++;

                    // Report only the first error in details
                    if (have_errors.exchange(trueVal))
                        return;

                    std::exception_ptr eptr = std::current_exception();
                    if (!eptr)
                        return;

                    try
                    {
                        rethrow_exception(eptr);
                    }
                    catch(...)
                    {
                        auto error = Fmi::Exception::Trace(BCP, "Error initializing pool item of type " +
                                Fmi::demangle_cpp_type_name(typeid(ItemType).name()))
                            .addParameter("Task name", name);
                        //std::cout << error << std::endl;
                        throw error;
                    }
                };

                AsyncTaskGroup task_group;
                task_group.stop_on_error(true);

                for (std::size_t i = 0; i < start_size; ++i)
                {
                    task_group.add("ini-pool-" + Fmi::to_string(i+1), grow);
                }

                task_group.on_task_error(init_task_error);
                task_group.wait();
            }
            else
            {
                throw Fmi::Exception(BCP, "Unsupported PoolInitType value");
            }
        }

        ItemRec* acquire(std::optional<Fmi::TimeDuration> timeout)
        {
            if (timeout && timeout->is_special())
                throw Fmi::Exception(BCP, "Special time values not supported as timeout value");

            const auto fetch_top = [](SharedState& s) -> ItemRec*
            {
                assert(s.top != nullptr);
                ItemRec* item_rec = s.top;
                s.top = s.top->next;
                item_rec->next = nullptr;
                item_rec->in_use = true;
                s.in_use_count++;
                return item_rec;
            };

            boost::unique_lock<boost::mutex> lock(state->mutex);

            // Check whether the pool is already destroyed
            if (!state->alive)
            {
                throw Fmi::Exception(BCP, "Pool is being destroyed");
            }

            if (state->top)
            {
                //---------------------------------------------------------------------
                // Item is available, use it
                //---------------------------------------------------------------------
                return fetch_top(*state);
            }
            else if (state->next_current_size < max_size)
            {
                //---------------------------------------------------------------------
                // Item is not available, max limit not exceeded, create a new item
                //---------------------------------------------------------------------
                // Update count while mutex is still locked to avoid growing over
                // max_size also in case of concurrent calls
                state->next_current_size++;
                // Unlock mutex while creating new item (it may take some time for example
                // of database connection)
                lock.unlock();
                std::unique_ptr<ItemType> new_item;
                try
                {
                    new_item = createItemCb();
                }
                catch (...)
                {
                    // Creating new item failed. Decrement count and rethrow
                    // the exception. This really only matters when pool expansion
                    // is attempted.
                    lock.lock();
                    state->next_current_size--;
                    throw;
                }
                // Update top and in_use_count while mutex is locked
                lock.lock();
                if (!state->alive)
                {
                    // Pool was destroyed while the new item was being created: discard it
                    state->next_current_size--;
                    throw Fmi::Exception(BCP, "Pool is being destroyed");
                }
                // Add new item to the pool. List iterators are not invalidated by growing list
                ItemRec& item_rec = state->pool_data.emplace_back(ItemRec(std::move(new_item)));
                // One could optimize this part by avoiding putting new item in free
                // item chain, but it would complicate the logic
                state->current_size++;
                item_rec.next = state->top;
                state->top = &item_rec;
                // No need to notify waiting threads as we are going to use the new item directly
                return fetch_top(*state);
            }
            else
            {
                // Also wake up when the pool is destroyed so that we do not wait forever
                const auto wake_condition = [this] { return state->top != nullptr || !state->alive; };

                if (timeout)
                {
                    int ms = static_cast<int>(timeout->total_milliseconds());
                    if (!state->cond_var.wait_for(lock, boost::chrono::milliseconds(ms), wake_condition))
                    {
                        throw Fmi::Exception(BCP, "Timeout while waiting for pool item");
                    }
                }
                else
                {
                    state->cond_var.wait(lock, wake_condition);
                }

                if (!state->alive)
                {
                    throw Fmi::Exception(BCP, "Pool is being destroyed");
                }

                if (state->top)
                {
                    //-----------------------------------------------------------------
                    // Item is available, use it
                    //-----------------------------------------------------------------
                    return fetch_top(*state);
                }

                // Should not be here
                throw Fmi::Exception(BCP, "Internal error");
            }
        }

        /**
         * @brief Return an item to the pool (called by Pool<>::Ptr deleter)
         *
         * Static method operating on the shared state only: safe to call also when the pool
         * object is already destroyed. In that case the item is not returned to the free item
         * chain; it is deleted when the last Ptr drops its reference to the shared state.
         */
        static void releaseItem(SharedState& state, ItemRec* item_rec)
        {
            boost::unique_lock<boost::mutex> lock(state.mutex);

            item_rec->in_use = false;
            state.in_use_count--;

            if (state.alive)
            {
                item_rec->next = state.top;
                state.top = item_rec;
                state.cond_var.notify_one();
            }
        }

        const std::size_t start_size;
        const std::size_t max_size;
        const std::tuple<typename std::decay<Args>::type...> constructor_args;

        std::function<std::unique_ptr<ItemType>()> createItemCb;

        std::shared_ptr<SharedState> state;
    };
}
