#pragma once

#include <boost/optional.hpp>
#include <boost/thread.hpp>
#include <map>
#include <string>

namespace Fmi
{
namespace Juche
{
typedef boost::mutex MutexType;
typedef boost::lock_guard<MutexType> Lock;

/** Cache with LRU and time eviction.

  Cache discards the least recently used (LRU) items first.
  Cache discards time expired items if constructed with
  a positive time eviction constant as seconds.
 */
template <typename KeyType, typename ValueType>
class Cache
{
  using TimeType = std::time_t;
  using TimeValuePair = std::pair<TimeType, ValueType>;
  using KeyTimeValuePair = std::pair<KeyType, TimeValuePair>;
  using KeyTimeValueList = std::list<KeyTimeValuePair>;
  using KeyTimeValueListIt = decltype(KeyTimeValueList().begin());
  using KeyKeyTimeValueListItPair = std::pair<KeyType, KeyTimeValueListIt>;
  using KeyTimeValueMap = std::map<KeyType, KeyTimeValueListIt>;

 public:
  // Constructor with LRU eviction and fixed cache size 10.
  Cache() : mMaxSize(10), mTimeConstant(0) {}

  // Constructor with LRU eviction and user defined cache size.
  Cache(const std::size_t& maxSize) : mMaxSize(maxSize), mTimeConstant(0) {}

  // Constructor with LRU eviction and time eviction and user defined cache size.
  Cache(const std::size_t& maxSize, const size_t& timeConstant)
      : mMaxSize(maxSize), mTimeConstant(timeConstant)
  {
  }

  bool insert(const KeyType& key, const ValueType& value)
  {
    Lock lock(mMutex);

    if (mMaxSize == 0) return false;

    auto now = std::time(NULL);
    auto mapIt = mKeyTimeValueMap.find(key);
    if (mapIt != mKeyTimeValueMap.end())
    {
      // Remove the object if expired, otherwise it is valid.
      auto listIt = mapIt->second;
      if (now - listIt->second.first > mTimeConstant)
      {
        mKeyTimeValueList.erase(listIt);
        mKeyTimeValueMap.erase(mapIt);
      }
      else
      {
        // Cache object is still valid.
        return false;
      }
    }

    // Object is not in the cache.
    // Make space if full and remove expired objects.
    if (mKeyTimeValueList.size() >= mMaxSize)
    {
      auto listIt = mKeyTimeValueList.rbegin();
      auto listEndIt = mKeyTimeValueList.rend();
      while ((mKeyTimeValueList.size() >= mMaxSize) or
             (mTimeConstant > 0 and (listIt != listEndIt) and
              (now - listIt->second.first > mTimeConstant)))
      {
        mapIt = mKeyTimeValueMap.find(listIt->first);
        if (mapIt != mKeyTimeValueMap.end())
        {
          mKeyTimeValueList.pop_back();
          mapIt = mKeyTimeValueMap.erase(mapIt);
        }

        listIt = mKeyTimeValueList.rbegin();
      }
    }

    if (mKeyTimeValueList.size() >= mMaxSize)
      throw std::runtime_error("Object cache is still full after cleaning");

    mKeyTimeValueList.push_front(KeyTimeValuePair(key, TimeValuePair(now, value)));
    mKeyTimeValueMap.insert(KeyKeyTimeValueListItPair(key, mKeyTimeValueList.begin()));

    return true;
  }

  boost::optional<ValueType> find(const KeyType& key) const
  {
    Lock lock(mMutex);

    if (mMaxSize == 0) return boost::optional<ValueType>();

    auto mapIt = mKeyTimeValueMap.find(key);
    if (mapIt == mKeyTimeValueMap.end()) return boost::optional<ValueType>();

    std::time_t now = std::time(NULL);
    auto listIt = mapIt->second;

    if (mTimeConstant > 0)
    {
      // Remove the object from the cache if expired
      if (now - listIt->second.first > mTimeConstant)
      {
        mKeyTimeValueList.erase(listIt);
        mKeyTimeValueMap.erase(mapIt);

        return boost::optional<ValueType>();
      }
    }

    // Move the object to the begin of the list (LRU).
    mKeyTimeValueList.splice(mKeyTimeValueList.begin(), mKeyTimeValueList, listIt);
    listIt = mKeyTimeValueList.begin();
    if (listIt->first != key) throw std::runtime_error("Mixed keys after a splice of cache list");

    // Update the reference of object to the map.
    mapIt = mKeyTimeValueMap.find(listIt->first);
    if (mapIt == mKeyTimeValueMap.end())
      throw std::runtime_error("Map object not found after a splice of cache list");
    mapIt->second = listIt;

    return boost::optional<ValueType>(listIt->second.second);
  }

  std::size_t size() const { return mKeyTimeValueMap.size(); }

  std::size_t maxSize() const { return mMaxSize; }

 private:
  mutable KeyTimeValueMap mKeyTimeValueMap;
  mutable KeyTimeValueList mKeyTimeValueList;
  mutable MutexType mMutex;

  std::size_t mMaxSize;

  std::size_t mTimeConstant;
};
}  // namespace Juche
}  // namespace Fmi
