#pragma once

#include <list>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

namespace Fmi
{
/**
 *   @brief Template class which implements object pool
 *
 *   Objects can be reused when only copy of shared pointer
 *   to the object belongs to ObjectPool instance.
 *
 *   It is user responsibility to ensure correct object
 *   state when reused
 *
 *   The actual number of objects in the pool may
 *   exceed specified limit. In this case extra objects
 *   will be relesed when number of actually used
 *   objects drops below this limit. The release
 *   of extra object is performed by
 *   - method get()
 *   - method update()
 */
template <typename ObjectType>
class ObjectPool : virtual protected boost::noncopyable
{
 public:
  /**
   *   @brief Constructor: creates a new object pool
   *
   *   @param create_new specifies call-back for creating a new object
   *         the object pool
   *   @param num_keep Specifies how many objects to keep
   *         when less than this number are being used.
   */
  ObjectPool(boost::function0<boost::shared_ptr<ObjectType> > create_new, std::size_t num_keep)
      : create_new(create_new), num_keep(num_keep)
  {
  }

  virtual ~ObjectPool() {}
  /**
   *   @brief Gets instance of an object (as boost::shared_ptr to it)
   */
  boost::shared_ptr<ObjectType> get()
  {
    auto object = find_unused();
    if (not object)
    {
      object = this->create_new();
      boost::mutex::scoped_lock lock(mutex);
      object_pool.push_front(object);
    }
    return object;
  }

  /**
   *   @brief Queries actual object count in the pool.
   */
  std::size_t size()
  {
    boost::mutex::scoped_lock lock(mutex);
    return object_pool.size();
  }

  /**
   *   @brief Update pool status: release extra unused objects if found
   */
  void update() { find_unused(); }
 private:
  boost::shared_ptr<ObjectType> find_unused()
  {
    std::size_t cnt = 0;
    boost::shared_ptr<ObjectType> object;
    std::list<boost::shared_ptr<ObjectType> > tmp;
    boost::mutex::scoped_lock lock(mutex);
    for (auto it = object_pool.begin(); it != object_pool.end(); ++it)
    {
      if (it->unique())
      {
        if (not object)
        {
          object = *it;
          tmp.push_back(*it);
          cnt++;
        }
        else if (cnt < num_keep)
        {
          tmp.push_back(*it);
          cnt++;
        }
      }
      else
      {
        tmp.push_back(*it);
        cnt++;
      }
    }

    object_pool.swap(tmp);

    return object;
  }

 private:
  boost::mutex mutex;
  std::list<boost::shared_ptr<ObjectType> > object_pool;
  boost::function0<boost::shared_ptr<ObjectType> > create_new;
  const std::size_t num_keep;
};
}
