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

template <typename KeyType, typename ValueType>
class Cache
{
 public:
  using TimeType = std::time_t;
  using TimeValuePair = std::pair<TimeType, ValueType>;
  using KeyTimeValuePair = std::pair<KeyType, TimeValuePair>;
  using KeyTimeValueMap = std::map<KeyType, TimeValuePair>;

  Cache() : mMaxSize(10), mTimeConstant(0) {}

  Cache(const std::size_t& maxSize, const long& timeConstant)
      : mMaxSize(maxSize), mTimeConstant(timeConstant)
  {
  }

  bool insert(const KeyType& key, const ValueType& value)
  {
    Lock lock(mMutex);

    if (mMaxSize == 0) return false;

    auto it = mKeyTimeValueMap.find(key);
    if (it != mKeyTimeValueMap.end())
    {
      if (std::time(NULL) - it->second.first > mTimeConstant)
      {
        it = mKeyTimeValueMap.erase(it);
      }
      else
      {
        return false;
      }
    }

    // Remove all expired
    if (mTimeConstant > 0 and mKeyTimeValueMap.size() >= mMaxSize)
    {
      for (auto it = mKeyTimeValueMap.begin(); it != mKeyTimeValueMap.end();)
      {
        if (std::time(NULL) - it->second.first > mTimeConstant)
        {
          it = mKeyTimeValueMap.erase(it);
        }
        else
          ++it;
      }
    }

    // Remove oldest
    if (mKeyTimeValueMap.size() >= mMaxSize)
    {
      auto oldestIt = mKeyTimeValueMap.begin();
      for (auto it = mKeyTimeValueMap.begin(); it != mKeyTimeValueMap.end(); ++it)
      {
        if (it->second.first < oldestIt->second.first) oldestIt = it;
      }

      if (oldestIt == mKeyTimeValueMap.end())
        throw std::runtime_error("Can not remove object from the full cache.");

      oldestIt = mKeyTimeValueMap.erase(oldestIt);
    }

    if (mKeyTimeValueMap.size() >= mMaxSize)
      throw std::runtime_error("Object cache is still full after cleaning");

    auto now = std::time(NULL);
    mKeyTimeValueMap.insert(KeyTimeValuePair(key, TimeValuePair(now, value)));

    return true;
  }

  // Find value from cache and remove it if it is too old.
  boost::optional<ValueType> find(const KeyType& key)
  {
    Lock lock(mMutex);

    auto it = mKeyTimeValueMap.find(key);
    if (it == mKeyTimeValueMap.end()) return boost::optional<ValueType>();

    std::time_t now = std::time(NULL);
    if (now - it->second.first > mTimeConstant)
    {
      it = mKeyTimeValueMap.erase(it);

      return boost::optional<ValueType>();
    }

    return boost::optional<ValueType>(it->second.second);
  }

  std::size_t size() const { return mKeyTimeValueMap.size(); }

  std::size_t maxSize() const { return mMaxSize; }

 private:
  KeyTimeValueMap mKeyTimeValueMap;

  MutexType mMutex;

  std::size_t mMaxSize;

  long mTimeConstant;
};
}  // namespace Juche
}  // namespace Fmi
