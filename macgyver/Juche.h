#pragma once

#include <string>
#include <iostream>
#include <boost/optional.hpp>
#include <boost/thread.hpp>
#include <map>

namespace Fmi
{
typedef boost::mutex MutexType;
typedef boost::lock_guard<MutexType> Lock;

template<typename KeyType, typename ValueType> class Juche
{
 public:
  using TimeType = std::time_t;
  using KeyTimePair = std::pair<KeyType, TimeType>;
  using KeyTimeMap = std::map<KeyType, TimeType>;

  using KeyValuePair = std::pair<KeyType, ValueType>;
  using KeyValueMap = std::map<KeyType, ValueType>;


  Juche() : mMaxSize(10), mTimeConstant(0) {}

  Juche(const std::size_t& maxSize, const long& timeConstant)
      : mMaxSize(maxSize), mTimeConstant(timeConstant)
  {
  }

  bool insert(const KeyType& key, const ValueType& value)
  {
    Lock lock(mMutex);

    if (mMaxSize == 0)
	    return false;

    auto ktIt = mKeyTimeMap.find(key);
    if (ktIt != mKeyTimeMap.end())
    {
	    if (std::time(NULL) - ktIt->second > mTimeConstant)
	    {
		    ktIt = mKeyTimeMap.erase(ktIt);

		    auto kvIt = mKeyValueMap.find(key);
		    if (kvIt != mKeyValueMap.end())
			    kvIt = mKeyValueMap.erase(kvIt);
	    }
	    else
	    {
		    return false;
	    }
    }

    // Remove all expired
    if (mKeyTimeMap.size() >= mMaxSize)
    {
	    for (auto ktIt = mKeyTimeMap.begin(); ktIt != mKeyTimeMap.end(); ++ktIt)
	    {
		    if (std::time(NULL) - ktIt->second > mTimeConstant)
		    {
			    auto kvIt = mKeyValueMap.find(ktIt->first);
			    if (kvIt != mKeyValueMap.end())
				    kvIt = mKeyValueMap.erase(kvIt);

			    ktIt = mKeyTimeMap.erase(ktIt);
		    }
	    }
    }


    // Remove oldest
    if (mKeyValueMap.size() >= mMaxSize)
    {
	    auto oldestKtIt = mKeyTimeMap.begin();
	    for (auto ktIt = mKeyTimeMap.begin(); ktIt != mKeyTimeMap.end(); ++ktIt)
	    {
		    if (ktIt->second < oldestKtIt->second)
			    oldestKtIt = ktIt;
	    }


	    if (oldestKtIt == mKeyTimeMap.end())
		    throw std::runtime_error("Can not remove object from the full cache.");


	    auto kvIt = mKeyValueMap.find(oldestKtIt->first);
	    if (kvIt == mKeyValueMap.end())
		    throw std::runtime_error("Cannot find value for the key of KeyTime map.");

	    kvIt = mKeyValueMap.erase(kvIt);
	    oldestKtIt = mKeyTimeMap.erase(oldestKtIt);
    }

    if (mKeyValueMap.size() >= mMaxSize)
	    throw std::runtime_error("Object cache is still full after cleaning");

    auto now = std::time(NULL);
    mKeyValueMap.insert(KeyValuePair(key, value));
    mKeyTimeMap.insert(KeyTimePair(key, now));

    if (mKeyValueMap.size() != mKeyTimeMap.size())
	    throw std::runtime_error("KeyValue and KeyTime mapping differs from each others.");

    return true;
  }

  // Find value from cache and remove it if it is too old.
  boost::optional<ValueType> find(const KeyType& key)
  {
    Lock lock(mMutex);

    auto ktIt = mKeyTimeMap.find(key);
    if (ktIt == mKeyTimeMap.end())
	    return boost::optional<ValueType>();


    auto kvIt = mKeyValueMap.find(key);

    std::time_t now = std::time(NULL);
    if (now - ktIt->second > mTimeConstant)
    {
	    ktIt = mKeyTimeMap.erase(ktIt);
	    if (kvIt != mKeyValueMap.end())
		    kvIt = mKeyValueMap.erase(kvIt);

	    return boost::optional<ValueType>();
    }

    return boost::optional<ValueType>(kvIt->second);
  }

  std::size_t size() const { return mKeyValueMap.size(); }

  std::size_t maxSize() const { return mMaxSize; }

 private:
  KeyValueMap mKeyValueMap;
  KeyTimeMap mKeyTimeMap;

  MutexType mMutex;

  std::size_t mMaxSize;

  long mTimeConstant;
};
}
