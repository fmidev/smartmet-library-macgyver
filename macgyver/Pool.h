#pragma once

#include "PostgreSQLConnection.h"
#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <list>
#include <mutex>
#include <condition_variable>
#include <exception>
#include "AsyncTaskGroup.h"
#include "DateTime.h"
#include "StringConversion.h"
#include "TypeName.h"

namespace Fmi
{
    enum class PoolInitType
    {
        Sequential,
        Parallel
    };

    template <PoolInitType InitType, typename ItemType, typename... Args>
    class Pool
    {
        struct ItemRec
        {
            /**
             * @brief Actual pool item
             */
            std::unique_ptr<ItemType> data;

            /**
             * @brief Pointer to the next free item record in the pool (nullptr if in use)
             */
            ItemRec* next;

            ItemRec(std::unique_ptr<ItemType>&& item)
                : data(std::move(item))
                , next(nullptr)
            {
            }

            ItemRec(const ItemRec&) = delete;
            ItemRec& operator=(const ItemRec&) = delete;
            ItemRec(ItemRec&&) = default;
            ItemRec& operator=(ItemRec&&) = default;
        };
    public:
        class Ptr : private std::unique_ptr<ItemType, std::function<void(ItemType*)>>
        {
            friend class Pool;

            Ptr(ItemType* ptr, std::function<void(ItemType*)> release)
                : std::unique_ptr<ItemType, std::function<void(ItemType*)>>(ptr, release)
            {
            }

            using base = std::unique_ptr<ItemType, std::function<void(ItemType*)>>;

        public:
            // We do not want to expose all std::unique_ptr<> methods for example release()
            inline operator ItemType*() const { return this->base::get(); }
            inline operator bool() const { return this->base::get() != nullptr; }
            inline ItemType* operator->() const { return this->base::get(); }
            inline ItemType* get() const { return this->base::get(); }
            inline void reset() { this->base::reset(); }
        };

        Pool(std::size_t start_size, std::size_t max_size, Args... args)
            : start_size(std::max(std::size_t(2), start_size))
            , max_size(std::max(start_size, max_size))
            , current_size(0)
            , in_use_count(0)
            , createItemCb([=]() { return std::make_unique<ItemType>(args...); })
        {
            init(args...);
        }

        Pool(
            std::function<std::unique_ptr<ItemType>(Args... args)> createItemCb,
            std::size_t start_size,
            std::size_t max_size,
            Args... args)

            : start_size(std::max(std::size_t(2), start_size))
            , max_size(std::max(start_size, max_size))
            , current_size(0)
            , in_use_count(0)
            , createItemCb(createItemCb)
        {
            init(args...);
        }

        virtual ~Pool()
        {
            if (in_use_count)
            {
                // FIXME: add waiting for pool items to be freed (with timeout)

                // There are some items in use. This will cause std::terminate
                // Unfortunately there is no way to recover from this without crashing
                Fmi::Exception error(BCP, "Pool is being destroyed while items are in use");
                std::cerr << error << std::endl;
                std::terminate();
            }
        }

        Ptr get()
        {
            ItemRec* rec = acquire(std::nullopt);
            return Ptr(rec->data.get(), [this, rec](ItemType* ptr) { release(rec); });
        }

        Ptr get(const Fmi::TimeDuration& timeout)
        {
            ItemRec* rec = acquire(timeout);
            return Ptr(rec->data.get(), [this, rec](ItemType* ptr) { release(rec); });
        }

        std::size_t size() const
        {
            std::unique_lock<std::mutex> lock(mutex);
            return current_size;
        }

        std::size_t in_use() const
        {
            std::unique_lock<std::mutex> lock(mutex);
            return in_use_count;
        }

        void dumpInfo(std::ostream& os)
        {
            int count = 0;
            os << "Pool info for items of type " << Fmi::demangle_cpp_type_name(typeid(ItemType).name()) << std::endl;
            os << "Total items: " << pool_data.size() << std::endl;
            os << "In use items: " << in_use_count << std::endl;
            os << "Top free item: " << (void*)top << std::endl;
            for (const auto& item : pool_data)
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
                std::unique_lock<std::mutex> lock(mutex);
                ItemRec& item_rec = pool_data.emplace_back(ItemRec(std::move(new_item)));
                item_rec.next = top;
                top = &item_rec;
                current_size++;
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
                        std::cout << Fmi::Exception(BCP,
                            "Error initializing pool item of type " +
                            Fmi::demangle_cpp_type_name(typeid(ItemType).name()));
                        }
                };

                AsyncTaskGroup task_group;
                for (std::size_t i = 0; i < start_size; ++i)
                {
                    task_group.add("pool_item_init[" + Fmi::to_string(i+1) + "]", grow);
                }

                task_group.on_task_error(init_task_error);
                task_group.wait();
                if (num_errors > 0)
                {
                    throw Fmi::Exception(BCP, Fmi::to_string(num_errors) +
                        " errors occurred while initializing pool");
                }
            }
        }

        ItemRec* acquire(std::optional<Fmi::TimeDuration> timeout)
        {
            if (timeout && timeout->is_special())
                throw Fmi::Exception(BCP, "Special time values not supported as timeout value");

            const auto fetch_top = [this]() -> ItemRec*
            {
                assert(top != nullptr);
                ItemRec* item_rec = top;
                top = top->next;
                item_rec->next = nullptr;
                in_use_count++;
                return item_rec;
            };

            std::unique_lock<std::mutex> lock(mutex);
            if (top)
            {
                //---------------------------------------------------------------------
                // Item is available, use it
                //---------------------------------------------------------------------
                return fetch_top();
            }
            else if (current_size < max_size)
            {
                //---------------------------------------------------------------------
                // Item is not available, max limit not exceeded, create a new item
                //---------------------------------------------------------------------
                // Update count while mutex is still locked to avoid growing over
                // max_size also in case of concurrent calls
                current_size++;
                // Unlock mutex while creating new item (it may take some time for example
                // of database connection)
                lock.unlock();
                std::unique_ptr<ItemType> new_item(createItemCb());
                // Update top and in_use_count while mutex is locked
                lock.lock();
                ItemRec& item_rec = pool_data.emplace_back(ItemRec(std::move(new_item)));
                // One could optimize this part by avoiding putting new item in free
                // item chain, but it would complicate the logic
                item_rec.next = top;
                top = &item_rec;

                return fetch_top();
            }
            else
            {
                if (timeout)
                {
                    if (!cond_var.wait_for(lock, timeout->get_impl(), [this] { return in_use_count < current_size; }))
                    {
                        throw Fmi::Exception(BCP, "Timeout while waiting for pool item");
                    }
                }
                else
                {
                    cond_var.wait(lock, [this] { return in_use_count < current_size; });
                }

                if (top)
                {
                    //-----------------------------------------------------------------
                    // Item is available, use it
                    //-----------------------------------------------------------------
                    return fetch_top();
                }

                // Should not be here
                throw Fmi::Exception(BCP, "Internal error");
            }
        }

        void release(ItemRec* item)
        {
            std::unique_lock<std::mutex> lock(mutex);
            item->next = top;
            top = item;
            in_use_count--;
            cond_var.notify_one();
        }


        const std::size_t start_size;
        const std::size_t max_size;
        std::size_t current_size = 0;
        std::size_t in_use_count = 0;

        mutable std::mutex mutex;
        std::condition_variable cond_var;

        std::function<std::unique_ptr<ItemType>()> createItemCb;

        ItemRec* top = nullptr;
        std::list<ItemRec> pool_data;
    };
}
