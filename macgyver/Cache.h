
// ======================================================================
/*!
 * \brief Tagged multi-strategy caching in multithreaded environment
 */
// ======================================================================
#pragma once

#include <boost/noncopyable.hpp>
#include <boost/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/bimap/list_of.hpp>
#include <boost/thread.hpp>
#include <boost/optional.hpp>
#include <boost/unordered_map.hpp>
#include <boost/functional/hash.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/spirit/include/qi.hpp>

// For random number generation
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/uniform_real_distribution.hpp>

#include <cmath>
#include <map>
#include <set>
#include <utility>
#include <limits>
#include <ctime>
#include <sstream>
#include <iterator>
#include <fstream>

// #ifndef NDEBUG
#if 0

#define FMI_CACHE_HIT ++cacheHits
#define FMI_CACHE_MISS ++cacheMisses
#define FMI_CACHE_EXPIRED_DELETED ++expiredDeleted
#define FMI_CACHE_EXPIRED_RETURNED ++expiredReturned
#define FMI_CACHE_EXPIRATION_FUNCTION(NAME, ...) \
  NAME(__VA_ARGS__, unsigned long& expiredDeleted, unsigned long& expiredReturned)
#define FMI_CACHE_EXPIRATION_CALL(NAME, ...) NAME(__VA_ARGS__, expiredReturned, expiredDeleted);

#else

#define FMI_CACHE_HIT
#define FMI_CACHE_MISS
#define FMI_CACHE_EXPIRED_DELETED
#define FMI_CACHE_EXPIRED_RETURNED
#define FMI_CACHE_EXPIRATION_FUNCTION(NAME, ...) NAME(__VA_ARGS__)
#define FMI_CACHE_EXPIRATION_CALL(NAME, ...) NAME(__VA_ARGS__)

#endif

namespace Fmi
{
namespace Cache
{
typedef boost::mutex MutexType;
typedef boost::lock_guard<MutexType> Lock;

// ----------------------------------------------------------------------
/*!
 * \brief Object which is stored in the cache
 */
// ----------------------------------------------------------------------

template <class KeyType, class ValueType, class TagSetType>
struct CacheObject
{
  CacheObject(const KeyType& theKey,
              const ValueType& theValue,
              const TagSetType& theSet,
              std::size_t theSize)
      : itsKey(theKey), itsValue(theValue), itsTagSet(theSet), itsHits(0), itsSize(theSize)
  {
  }

  KeyType itsKey;

  ValueType itsValue;

  TagSetType itsTagSet;

  std::size_t itsHits;

  std::size_t itsSize;
};

template <class KeyType, class ValueType, class TagSetType>
struct CacheReportingObject
{
  CacheReportingObject(const KeyType& theKey,
                       const ValueType& theValue,
                       const TagSetType& theSet,
                       std::size_t theHits,
                       std::size_t theSize)
      : itsKey(theKey), itsValue(theValue), itsTagSet(theSet), itsHits(theHits), itsSize(theSize)
  {
  }

  KeyType itsKey;

  ValueType itsValue;

  TagSetType itsTagSet;

  std::size_t itsHits;

  std::size_t itsSize;
};

// ----------------------------------------------------------------------
/*!
 * \brief Eviction types which determine how cache is evicted. Eviction
 * can be either object or size based.
 */
// ----------------------------------------------------------------------

template <class ValueType>
struct TrivialSizeFunction
{
  static std::size_t getSize(const ValueType& theValue) { return 1; }
};

template <class TagSetType, class TagMapType>
void perform_tag_eviction(const TagSetType& tags, TagMapType& inputTagMap)
{
  for (auto tagit = tags.begin(); tagit != tags.end(); ++tagit)
  {
    auto mapit = inputTagMap.find(*tagit);

    if (mapit != inputTagMap.end())
    {
      // Decrease ownership count
      mapit->second.second--;

      if (mapit->second.second == 0)
      {
        inputTagMap.erase(mapit);
      }
    }
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Eviction policies determine how cache is cleaned upon reaching the maximum size.
 */
// ----------------------------------------------------------------------

// Remove the least used object from the cache
template <class MapType,
          class TagMapType,
          class KeyType,
          class ValueType,
          class TagSetType,
          class SizeFunction>
struct LRUEviction
{
  static void onEvict(MapType& inputMap,
                      TagMapType& inputTagMap,
                      std::size_t& currentSize,
                      std::size_t maxSize)
  {
    while (currentSize > maxSize)
    {
      std::size_t valueSize = inputMap.right.front().first.itsSize;
      TagSetType valueTags = inputMap.right.front().first.itsTagSet;

      perform_tag_eviction<TagSetType, TagMapType>(valueTags, inputTagMap);

      inputMap.right.pop_front();
      currentSize -= valueSize;
    }
  }

  static void onEvict(MapType& inputMap,
                      TagMapType& inputTagMap,
                      std::size_t& currentSize,
                      std::size_t maxSize,
                      std::vector<std::pair<KeyType, ValueType> >& evictedItems)
  {
    while (currentSize > maxSize)
    {
      auto& item = inputMap.right.front().first;
      std::size_t valueSize = item.itsSize;
      TagSetType valueTags = item.itsTagSet;

      perform_tag_eviction<TagSetType, TagMapType>(valueTags, inputTagMap);

      evictedItems.push_back(std::make_pair(item.itsKey, item.itsValue));

      inputMap.right.pop_front();
      currentSize -= valueSize;
    }
  }

  static bool onInsert(MapType& inputMap,
                       TagMapType& inputTagMap,
                       const KeyType& key,
                       const ValueType& value,
                       const TagSetType& tags,
                       std::size_t& currentSize,
                       std::size_t maxSize)
  {
    std::size_t amountToInsert = SizeFunction::getSize(value);

    if (amountToInsert >= maxSize)
    {
      // User attempted to insert something that is bigger than the entire cache, return fail
      return false;
    }

    currentSize += amountToInsert;

    // Evict until within the maxSize limit
    onEvict(inputMap, inputTagMap, currentSize, maxSize);

    inputMap.insert(typename MapType::value_type(
        key, CacheObject<KeyType, ValueType, TagSetType>(key, value, tags, amountToInsert)));

    return true;
  }

  static bool onInsert(MapType& inputMap,
                       TagMapType& inputTagMap,
                       const KeyType& key,
                       const ValueType& value,
                       const TagSetType& tags,
                       std::size_t& currentSize,
                       std::size_t maxSize,
                       std::vector<std::pair<KeyType, ValueType> >& evictedItems)
  {
    std::size_t amountToInsert = SizeFunction::getSize(value);

    if (amountToInsert >= maxSize)
    {
      // User attempted to insert something that is bigger than the entire cache, return fail
      return false;
    }

    currentSize += amountToInsert;

    // Evict until within the maxSize limit
    onEvict(inputMap, inputTagMap, currentSize, maxSize, evictedItems);

    inputMap.insert(typename MapType::value_type(
        key, CacheObject<KeyType, ValueType, TagSetType>(key, value, tags, amountToInsert)));

    return true;
  }

  static void onAccess(MapType& inputMap, typename MapType::left_iterator& it)
  {
    inputMap.right.relocate(inputMap.right.end(), inputMap.project_right(it));
  }
};

// Remove the most used object from the cache
template <class MapType,
          class TagMapType,
          class KeyType,
          class ValueType,
          class TagSetType,
          class SizeFunction>
struct MRUEviction
{
  static void onEvict(MapType& inputMap,
                      TagMapType& inputTagMap,
                      std::size_t& currentSize,
                      std::size_t maxSize)
  {
    while (currentSize > maxSize)
    {
      std::size_t valueSize = inputMap.right.back().first.itsSize;
      TagSetType valueTags = inputMap.right.back().first.itsTagSet;

      perform_tag_eviction<TagSetType, TagMapType>(valueTags, inputTagMap);

      inputMap.right.pop_back();
      currentSize -= valueSize;
    }
  }

  static void onEvict(MapType& inputMap,
                      TagMapType& inputTagMap,
                      std::size_t& currentSize,
                      std::size_t maxSize,
                      std::vector<std::pair<KeyType, ValueType> >& evictedItems)
  {
    while (currentSize > maxSize)
    {
      auto& item = inputMap.right.back().first;
      std::size_t valueSize = item.itsSize;
      TagSetType valueTags = item.itsTagSet;

      evictedItems.push_back(std::make_pair(item.itsKey, item.itsValue));

      perform_tag_eviction<TagSetType, TagMapType>(valueTags, inputTagMap);

      inputMap.right.pop_back();
      currentSize -= valueSize;
    }
  }

  static bool onInsert(MapType& inputMap,
                       TagMapType& inputTagMap,
                       const KeyType& key,
                       const ValueType& value,
                       const TagSetType& tags,
                       std::size_t& currentSize,
                       std::size_t maxSize)
  {
    std::size_t amountToInsert = SizeFunction::getSize(value);

    if (amountToInsert >= maxSize)
    {
      // User attempted to insert something that is bigger than the entire cache, return fail
      return false;
    }

    currentSize += amountToInsert;

    onEvict(inputMap, inputTagMap, currentSize, maxSize);

    inputMap.insert(typename MapType::value_type(
        key, CacheObject<KeyType, ValueType, TagSetType>(key, value, tags, amountToInsert)));

    return true;
  }

  static bool onInsert(MapType& inputMap,
                       TagMapType& inputTagMap,
                       const KeyType& key,
                       const ValueType& value,
                       const TagSetType& tags,
                       std::size_t& currentSize,
                       std::size_t maxSize,
                       std::vector<std::pair<KeyType, ValueType> >& evictedItems)
  {
    std::size_t amountToInsert = SizeFunction::getSize(value);

    if (amountToInsert >= maxSize)
    {
      // User attempted to insert something that is bigger than the entire cache, return fail
      return false;
    }

    currentSize += amountToInsert;

    onEvict(inputMap, inputTagMap, currentSize, maxSize, evictedItems);

    inputMap.insert(typename MapType::value_type(
        key, CacheObject<KeyType, ValueType, TagSetType>(key, value, tags, amountToInsert)));

    return true;
  }

  static void onAccess(MapType& inputMap, typename MapType::left_iterator& it)
  {
    inputMap.right.relocate(inputMap.right.end(), inputMap.project_right(it));
  }
};

// Remove randomly chosen object from the cache
template <class MapType,
          class TagMapType,
          class KeyType,
          class ValueType,
          class TagSetType,
          class SizeFunction>
struct RandomEviction
{
  static void onEvict(MapType& inputMap,
                      TagMapType& inputTagMap,
                      std::size_t& currentSize,
                      std::size_t maxSize)
  {
    static boost::random::mt19937 generator(time(NULL));

    while (currentSize > maxSize)
    {
      std::size_t mapSize = inputMap.size();
      boost::random::uniform_int_distribution<> dist(0, mapSize - 1);
      std::size_t randomInteger = dist(generator);
      auto iterator = inputMap.right.begin();
      std::advance(iterator, randomInteger);

      std::size_t valueSize = iterator->first.itsSize;
      TagSetType valueTags = iterator->first.itsTagSet;

      perform_tag_eviction<TagSetType, TagMapType>(valueTags, inputTagMap);

      inputMap.right.erase(iterator);
      currentSize -= valueSize;
    }
  }

  static void onEvict(MapType& inputMap,
                      TagMapType& inputTagMap,
                      std::size_t& currentSize,
                      std::size_t maxSize,
                      std::vector<std::pair<KeyType, ValueType> >& evictedItems)
  {
    static boost::random::mt19937 generator(time(NULL));

    while (currentSize > maxSize)
    {
      std::size_t mapSize = inputMap.size();
      boost::random::uniform_int_distribution<> dist(0, mapSize - 1);
      std::size_t randomInteger = dist(generator);
      auto iterator = inputMap.right.begin();
      std::advance(iterator, randomInteger);

      auto& item = iterator->first;
      std::size_t valueSize = item.itsSize;
      TagSetType valueTags = item.itsTagSet;

      perform_tag_eviction<TagSetType, TagMapType>(valueTags, inputTagMap);

      evictedItems.push_back(std::make_pair(item.itsKey, item.itsValue));

      inputMap.right.erase(iterator);
      currentSize -= valueSize;
    }
  }

  static bool onInsert(MapType& inputMap,
                       TagMapType& inputTagMap,
                       const KeyType& key,
                       const ValueType& value,
                       const TagSetType& tags,
                       std::size_t& currentSize,
                       std::size_t maxSize)
  {
    static boost::random::mt19937 generator(time(NULL));

    std::size_t amountToInsert = SizeFunction::getSize(value);

    if (amountToInsert >= maxSize)
    {
      // User attempted to insert something that is bigger than the entire cache, return fail
      return false;
    }

    currentSize += amountToInsert;

    // Evict until within the maxSize limit
    onEvict(inputMap, inputTagMap, currentSize, maxSize);

    inputMap.insert(typename MapType::value_type(
        key, CacheObject<KeyType, ValueType, TagSetType>(key, value, tags, amountToInsert)));

    return true;
  }

  static bool onInsert(MapType& inputMap,
                       TagMapType& inputTagMap,
                       const KeyType& key,
                       const ValueType& value,
                       const TagSetType& tags,
                       std::size_t& currentSize,
                       std::size_t maxSize,
                       std::vector<std::pair<KeyType, ValueType> >& evictedItems)
  {
    static boost::random::mt19937 generator(time(NULL));

    std::size_t amountToInsert = SizeFunction::getSize(value);

    if (amountToInsert >= maxSize)
    {
      // User attempted to insert something that is bigger than the entire cache, return fail
      return false;
    }

    currentSize += amountToInsert;

    // Evict until within the maxSize limit
    onEvict(inputMap, inputTagMap, currentSize, maxSize, evictedItems);

    inputMap.insert(typename MapType::value_type(
        key, CacheObject<KeyType, ValueType, TagSetType>(key, value, tags, amountToInsert)));

    return true;
  }

  static void onAccess(MapType& inputMap, typename MapType::left_iterator& it)
  {
    // No-op
  }
};

// First in first out removal policy
template <class MapType,
          class TagMapType,
          class KeyType,
          class ValueType,
          class TagSetType,
          class SizeFunction>
struct FIFOEviction
{
  static void onEvict(MapType& inputMap,
                      TagMapType& inputTagMap,
                      std::size_t& currentSize,
                      std::size_t maxSize)
  {
    while (currentSize > maxSize)
    {
      std::size_t valueSize = inputMap.right.front().first.itsSize;
      TagSetType valueTags = inputMap.right.front().first.itsTagSet;

      perform_tag_eviction<TagSetType, TagMapType>(valueTags, inputTagMap);

      inputMap.right.pop_front();
      currentSize -= valueSize;
    }
  }

  static void onEvict(MapType& inputMap,
                      TagMapType& inputTagMap,
                      std::size_t& currentSize,
                      std::size_t maxSize,
                      std::vector<std::pair<KeyType, ValueType> >& evictedItems)
  {
    while (currentSize > maxSize)
    {
      auto& item = inputMap.right.front().first;
      std::size_t valueSize = item.itsSize;
      TagSetType valueTags = item.itsTagSet;

      perform_tag_eviction<TagSetType, TagMapType>(valueTags, inputTagMap);

      evictedItems.push_back(std::make_pair(item.itsKey, item.itsValue));

      inputMap.right.pop_front();
      currentSize -= valueSize;
    }
  }

  static bool onInsert(MapType& inputMap,
                       TagMapType& inputTagMap,
                       const KeyType& key,
                       const ValueType& value,
                       const TagSetType& tags,
                       std::size_t& currentSize,
                       std::size_t maxSize)
  {
    std::size_t amountToInsert = SizeFunction::getSize(value);

    if (amountToInsert >= maxSize)
    {
      // User attempted to insert something that is bigger than the entire cache, return fail
      return false;
    }

    currentSize += amountToInsert;

    // Evict until within the maxSize limit
    onEvict(inputMap, inputTagMap, currentSize, maxSize);

    inputMap.insert(typename MapType::value_type(
        key, CacheObject<KeyType, ValueType, TagSetType>(key, value, tags, amountToInsert)));

    return true;
  }

  static bool onInsert(MapType& inputMap,
                       TagMapType& inputTagMap,
                       const KeyType& key,
                       const ValueType& value,
                       const TagSetType& tags,
                       std::size_t& currentSize,
                       std::size_t maxSize,
                       std::vector<std::pair<KeyType, ValueType> >& evictedItems)
  {
    std::size_t amountToInsert = SizeFunction::getSize(value);

    if (amountToInsert >= maxSize)
    {
      // User attempted to insert something that is bigger than the entire cache, return fail
      return false;
    }

    currentSize += amountToInsert;

    // Evict until within the maxSize limit
    onEvict(inputMap, inputTagMap, currentSize, maxSize, evictedItems);

    inputMap.insert(typename MapType::value_type(
        key, CacheObject<KeyType, ValueType, TagSetType>(key, value, tags, amountToInsert)));

    return true;
  }

  static void onAccess(MapType& inputMap, typename MapType::left_iterator& it)
  {
    // No-op
  }
};

// First in last out removal policy
template <class MapType,
          class TagMapType,
          class KeyType,
          class ValueType,
          class TagSetType,
          class SizeFunction>
struct FILOEviction
{
  static void onEvict(MapType& inputMap,
                      TagMapType& inputTagMap,
                      std::size_t& currentSize,
                      std::size_t maxSize)
  {
    while (currentSize > maxSize)
    {
      std::size_t valueSize = inputMap.right.back().first.itsSize;
      TagSetType valueTags = inputMap.right.back().first.itsTagSet;

      perform_tag_eviction<TagSetType, TagMapType>(valueTags, inputTagMap);

      inputMap.right.pop_back();
      currentSize -= valueSize;
    }
  }

  static void onEvict(MapType& inputMap,
                      TagMapType& inputTagMap,
                      std::size_t& currentSize,
                      std::size_t maxSize,
                      std::vector<std::pair<KeyType, ValueType> >& evictedItems)
  {
    while (currentSize > maxSize)
    {
      auto& item = inputMap.right.back().first;
      std::size_t valueSize = item.itsSize;
      TagSetType valueTags = item.itsTagSet;

      perform_tag_eviction<TagSetType, TagMapType>(valueTags, inputTagMap);

      evictedItems.push_back(std::make_pair(item.itsKey, item.itsValue));

      inputMap.right.pop_back();
      currentSize -= valueSize;
    }
  }

  static bool onInsert(MapType& inputMap,
                       TagMapType& inputTagMap,
                       const KeyType& key,
                       const ValueType& value,
                       const TagSetType& tags,
                       std::size_t& currentSize,
                       std::size_t maxSize)
  {
    std::size_t amountToInsert = SizeFunction::getSize(value);

    if (amountToInsert >= maxSize)
    {
      // User attempted to insert something that is bigger than the entire cache, return fail
      return false;
    }

    currentSize += amountToInsert;

    // Evict until within the maxSize limit
    onEvict(inputMap, inputTagMap, currentSize, maxSize);

    inputMap.insert(typename MapType::value_type(
        key, CacheObject<KeyType, ValueType, TagSetType>(key, value, tags, amountToInsert)));

    return true;
  }

  static bool onInsert(MapType& inputMap,
                       TagMapType& inputTagMap,
                       const KeyType& key,
                       const ValueType& value,
                       const TagSetType& tags,
                       std::size_t& currentSize,
                       std::size_t maxSize,
                       std::vector<std::pair<KeyType, ValueType> >& evictedItems)
  {
    std::size_t amountToInsert = SizeFunction::getSize(value);

    if (amountToInsert >= maxSize)
    {
      // User attempted to insert something that is bigger than the entire cache, return fail
      return false;
    }

    currentSize += amountToInsert;

    // Evict until within the maxSize limit
    onEvict(inputMap, inputTagMap, currentSize, maxSize, evictedItems);

    inputMap.insert(typename MapType::value_type(
        key, CacheObject<KeyType, ValueType, TagSetType>(key, value, tags, amountToInsert)));

    return true;
  }

  static void onAccess(MapType& inputMap, typename MapType::left_iterator& it)
  {
    // No-op
  }
};

// ----------------------------------------------------------------------
/*!
 * \brief Tag expiration policies. theTagTime is expiration time stamp for the given tag
 */
// ----------------------------------------------------------------------

// Tag expiration does not depend on expiration age
struct StaticExpire
{
  static bool FMI_CACHE_EXPIRATION_FUNCTION(isExpired,
                                            const std::time_t& theTagTime,
                                            long timeConstant)
  {
    // Max value means tag is valid
    if (theTagTime == std::numeric_limits<std::time_t>::max())
    {
      return false;
    }

    FMI_CACHE_EXPIRED_DELETED;
    return true;
  }

  // All expired tags are deleted
  static bool toDelete(const std::time_t& theTagTime, long timeConstant)
  {
    if (theTagTime != std::numeric_limits<std::time_t>::max())
    {
      return true;
    }
    else
    {
      return false;
    }
  }
};

// Tag expires instantly after expiration age is reached
struct InstantExpire
{
  static bool FMI_CACHE_EXPIRATION_FUNCTION(isExpired,
                                            const std::time_t& theTagTime,
                                            long timeConstant)
  {
    // Max value means tag is valid
    if (theTagTime == std::numeric_limits<std::time_t>::max())
    {
      return false;
    }

    std::time_t now = std::time(NULL);

    if ((now - theTagTime) > timeConstant)
    {
      FMI_CACHE_EXPIRED_DELETED;
      return true;
    }
    else
    {
      return false;
    }
  }

  // Expired tags older than timeConstant are deleted
  static bool toDelete(const std::time_t& theTagTime, long timeConstant)
  {
    std::time_t now = std::time(NULL);

    if ((now - theTagTime) > timeConstant)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
};

struct CoinFlipExpire
{
  static bool FMI_CACHE_EXPIRATION_FUNCTION(isExpired,
                                            const std::time_t& theTagTime,
                                            long timeConstant)
  {
    static boost::random::mt19937 generator(static_cast<unsigned int>(std::time(NULL)));
    static boost::random::uniform_int_distribution<> dist(0, 1);

    // Max value means tag is valid
    if (theTagTime == std::numeric_limits<std::time_t>::max())
    {
      return false;
    }

    std::time_t now = std::time(NULL);

    if ((now - theTagTime) > timeConstant)
    {
      int chance = dist(generator);
      if (chance == 0)
      {
        FMI_CACHE_EXPIRED_DELETED;
        return true;
      }
      else
      {
        FMI_CACHE_EXPIRED_RETURNED;
        return false;
      }
    }
    else
    {
      return false;
    }
  }

  // Expired tags older than 2*timeConstant are deleted
  static bool toDelete(const std::time_t& theTagTime, long timeConstant)
  {
    std::time_t now = std::time(NULL);

    if ((now - theTagTime) > 2 * timeConstant)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
};

// Linearly time-dependent expiration
struct LinearTimeExpire
{
  static bool FMI_CACHE_EXPIRATION_FUNCTION(isExpired,
                                            const std::time_t& theTagTime,
                                            long timeConstant)
  {
    static boost::random::mt19937 generator(static_cast<unsigned int>(std::time(NULL)));
    static boost::random::uniform_real_distribution<> dist(0.0, 1.0);

    // Max value means tag is valid
    if (theTagTime == std::numeric_limits<std::time_t>::max())
    {
      return false;
    }

    std::time_t now = std::time(NULL);
    long tagAge = now - theTagTime;

    assert(timeConstant != 0);

    double expirationProbability = double(tagAge) / double(timeConstant);
    double chance = dist(generator);
    if (chance < expirationProbability)
    {
      FMI_CACHE_EXPIRED_DELETED;
      return true;
    }
    else
    {
      FMI_CACHE_EXPIRED_RETURNED;
      return false;
    }
  }

  // Expired tags with >100% removal probability are deleted
  static bool toDelete(const std::time_t& theTagTime, long timeConstant)
  {
    std::time_t now = std::time(NULL);

    if ((now - theTagTime) > timeConstant)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
};

// Sigmoidal time-dependent expiration
struct SigmoidTimeExpire
{
  static bool FMI_CACHE_EXPIRATION_FUNCTION(isExpired,
                                            const std::time_t& theTagTime,
                                            long timeConstant)
  {
    static boost::random::mt19937 generator(static_cast<unsigned int>(time(NULL)));
    static boost::random::uniform_real_distribution<> dist(0.0, 1.0);
    // Max value means tag is valid
    if (theTagTime == std::numeric_limits<std::time_t>::max())
    {
      return false;
    }

    std::time_t now = std::time(NULL);
    long tagAge = now - theTagTime;

    double expirationProbability =
        1.0 /
        (1.0 + std::exp(-0.02 * static_cast<double>(tagAge) + static_cast<double>(timeConstant)));
    double chance = dist(generator);
    if (chance < expirationProbability)
    {
      FMI_CACHE_EXPIRED_DELETED;
      return true;
    }
    else
    {
      FMI_CACHE_EXPIRED_RETURNED;
      return false;
    }
  }

  // Expired tags are removed when their return probability is less than 1%
  static bool toDelete(const std::time_t& theTagTime, long timeConstant)
  {
    static double eliminationProbability = 0.99;
    static double eliminationAge =
        (std::log(eliminationProbability / (1.0 - eliminationProbability)) + 0.02) /
        double(timeConstant);
    std::time_t now = std::time(NULL);

    if ((now - theTagTime) > eliminationAge)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
};

// ----------------------------------------------------------------------
/*!
 * \brief Cache object supporting tags and various eviction and expiration policies
 */
// ----------------------------------------------------------------------

template <class KeyType,
          class ValueType,
          template <class, class, class, class, class, class> class EvictionPolicy = LRUEviction,
          class TagType = int,
          class ExpirationPolicy = StaticExpire,
          class SizeFunc = TrivialSizeFunction<ValueType> >
class Cache : public boost::noncopyable
{
 public:
  typedef std::set<TagType> TagSetType;
  typedef CacheObject<KeyType, ValueType, TagSetType> CacheObjectType;
  typedef CacheReportingObject<KeyType, ValueType, TagSetType> CacheReportingObjectType;

  typedef typename boost::bimaps::bimap<
      boost::bimaps::unordered_set_of<KeyType, boost::hash<KeyType>, std::equal_to<KeyType> >,
      boost::bimaps::list_of<CacheObjectType> > MapType;
  typedef typename MapType::left_iterator LeftIteratorType;
  typedef typename MapType::right_iterator RightIteratorType;

  typedef std::map<TagType, std::pair<std::time_t, std::size_t> > TagMapType;
  typedef std::vector<std::pair<KeyType, ValueType> > ItemVector;

  // Default constructor eases the use as data member
  Cache()
      : itsSize(0),
        itsMaxSize(10),
        itsTimeConstant(0)
#ifndef NDEBUG
        ,
        cacheHits(0),
        cacheMisses(0),
        expiredReturned(0),
        expiredDeleted(0)
#endif
  {
    //		itsMap.bucket_size(itsMaxSize);
  }

  Cache(std::size_t maxSize, long timeConstant = 0)
      : itsSize(0),
        itsMaxSize(maxSize),
        itsTimeConstant(timeConstant)
#ifndef NDEBUG
        ,
        cacheHits(0),
        cacheMisses(0),
        expiredReturned(0),
        expiredDeleted(0)
#endif
  {
    //		itsMap.bucket_size(itsMaxSize);
  }

  // Insert with a list of tags
  template <class ListType>
  bool insert(const KeyType& key, const ValueType& value, const ListType& tags)
  {
    Lock wlock(itsMutex);

    bool result;

    LeftIteratorType it = itsMap.left.find(key);

    if (it == itsMap.left.end())
    {
      // Value not in cache

      // Make tagSet
      TagSetType tagSet;
      for (auto it = tags.begin(); it != tags.end(); ++it)
      {
        tagSet.insert(*it);
      }

      // Perform insertion
      result =
          EvictionPolicy<MapType, TagMapType, KeyType, ValueType, TagSetType, SizeFunc>::onInsert(
              itsMap, itsTagMap, key, value, tagSet, itsSize, itsMaxSize);

      if (result)
      {
        // Successful insertion
        for (auto it = tags.begin(); it != tags.end(); ++it)
        {
          // Check and update tags
          // Check if tag exists in tag map, if not, insert default  value
          auto tagIt = itsTagMap.find(*it);
          if (tagIt == itsTagMap.end())
          {
            itsTagMap.insert(typename TagMapType::value_type(
                *it, std::make_pair(std::numeric_limits<std::time_t>::max(), 1)));
          }
          else
          {
            // Check if user is reusing an expired tag. If so, update the tag
            if (ExpirationPolicy::toDelete(tagIt->second.first, itsTimeConstant))
            {
              tagIt->second.first = std::numeric_limits<std::time_t>::max();
            }

            // Increase the usage counter
            tagIt->second.second++;
          }
        }
      }
    }

    else
    {
      // Value already in cache, not inserted
      result = false;
    }

    return result;
  }

  // Insert with a list of tags, return evicted items
  template <class ListType>
  bool insert(const KeyType& key,
              const ValueType& value,
              const ListType& tags,
              std::vector<std::pair<KeyType, ValueType> >& evictedItems)
  {
    evictedItems.clear();

    Lock wlock(itsMutex);

    bool result;

    LeftIteratorType it = itsMap.left.find(key);

    if (it == itsMap.left.end())
    {
      // Value not in cache

      // Make tagSet
      TagSetType tagSet;
      for (auto it = tags.begin(); it != tags.end(); ++it)
      {
        tagSet.insert(*it);
      }

      // Perform insertion
      result =
          EvictionPolicy<MapType, TagMapType, KeyType, ValueType, TagSetType, SizeFunc>::onInsert(
              itsMap, itsTagMap, key, value, tagSet, itsSize, itsMaxSize, evictedItems);

      if (result)
      {
        // Successful insertion
        for (auto it = tags.begin(); it != tags.end(); ++it)
        {
          // Check and update tags
          // Check if tag exists in tag map, if not, insert default  value
          auto tagIt = itsTagMap.find(*it);
          if (tagIt == itsTagMap.end())
          {
            itsTagMap.insert(typename TagMapType::value_type(
                *it, std::make_pair(std::numeric_limits<std::time_t>::max(), 1)));
          }
          else
          {
            // Check if user is reusing an expired tag. If so, update the tag
            if (ExpirationPolicy::toDelete(tagIt->second.first, itsTimeConstant))
            {
              tagIt->second.first = std::numeric_limits<std::time_t>::max();
            }

            // Increase the usage counter
            tagIt->second.second++;
          }
        }
      }
    }

    else
    {
      // Value already in cache, not inserted
      result = false;
    }

    return result;
  }

  // Insert with a single tag
  bool insert(const KeyType& key, const ValueType& value, const TagType& tag)
  {
    Lock wlock(itsMutex);

    bool result;

    LeftIteratorType it = itsMap.left.find(key);

    if (it == itsMap.left.end())
    {
      // Value not in cache

      TagSetType tagSet;
      tagSet.insert(tag);

      // Perform insertion
      result =
          EvictionPolicy<MapType, TagMapType, KeyType, ValueType, TagSetType, SizeFunc>::onInsert(
              itsMap, itsTagMap, key, value, tagSet, itsSize, itsMaxSize);

      if (result)
      {
        // Successful insertion
        // Check if tag exists in tag map, if not, insert default  value
        auto tagIt = itsTagMap.find(tag);
        if (tagIt == itsTagMap.end())
        {
          itsTagMap.insert(typename TagMapType::value_type(
              tag, std::make_pair(std::numeric_limits<std::time_t>::max(), 1)));
        }
        else
        {
          // Check if user is reusing an expired tag. If so, update the tag
          if (ExpirationPolicy::toDelete(tagIt->second.first, itsTimeConstant))
          {
            tagIt->second.first = std::numeric_limits<std::time_t>::max();
          }

          // Increase the usage counter
          tagIt->second.second++;
        }
      }
    }

    else
    {
      // Value already in cache, not inserted
      result = false;
    }

    return result;
  }

  // Insert with a single tag, returns evicted items
  bool insert(const KeyType& key,
              const ValueType& value,
              const TagType& tag,
              std::vector<std::pair<KeyType, ValueType> >& evictedItems)
  {
    evictedItems.clear();

    Lock wlock(itsMutex);

    bool result;

    LeftIteratorType it = itsMap.left.find(key);

    if (it == itsMap.left.end())
    {
      // Value not in cache

      TagSetType tagSet;
      tagSet.insert(tag);

      // Perform insertion
      result =
          EvictionPolicy<MapType, TagMapType, KeyType, ValueType, TagSetType, SizeFunc>::onInsert(
              itsMap, itsTagMap, key, value, tagSet, itsSize, itsMaxSize, evictedItems);

      if (result)
      {
        // Successful insertion
        // Check if tag exists in tag map, if not, insert default  value
        auto tagIt = itsTagMap.find(tag);
        if (tagIt == itsTagMap.end())
        {
          itsTagMap.insert(typename TagMapType::value_type(
              tag, std::make_pair(std::numeric_limits<std::time_t>::max(), 1)));
        }
        else
        {
          // Check if user is reusing an expired tag. If so, update the tag
          if (ExpirationPolicy::toDelete(tagIt->second.first, itsTimeConstant))
          {
            tagIt->second.first = std::numeric_limits<std::time_t>::max();
          }

          // Increase the usage counter
          tagIt->second.second++;
        }
      }
    }

    else
    {
      // Value already in cache, not inserted
      result = false;
    }

    return result;
  }

  // Tagless insert for simple use
  bool insert(const KeyType& key, const ValueType& value)
  {
    Lock lock(itsMutex);

    bool result;

    LeftIteratorType it = itsMap.left.find(key);

    if (it == itsMap.left.end())
    {
      // Value not in cache
      // Perform insertion
      result =
          EvictionPolicy<MapType, TagMapType, KeyType, ValueType, TagSetType, SizeFunc>::onInsert(
              itsMap, itsTagMap, key, value, TagSetType(), itsSize, itsMaxSize);
    }

    else
    {
      // Value already in cache, not inserted
      result = false;
    }

    return result;
  }

  // Tagless insert for simple use, returns evicted items
  bool insert(const KeyType& key,
              const ValueType& value,
              std::vector<std::pair<KeyType, ValueType> >& evictedItems)
  {
    evictedItems.clear();

    Lock lock(itsMutex);

    bool result;

    LeftIteratorType it = itsMap.left.find(key);

    if (it == itsMap.left.end())
    {
      // Value not in cache
      // Perform insertion
      result =
          EvictionPolicy<MapType, TagMapType, KeyType, ValueType, TagSetType, SizeFunc>::onInsert(
              itsMap, itsTagMap, key, value, TagSetType(), itsSize, itsMaxSize, evictedItems);
    }

    else
    {
      // Value already in cache, not inserted
      result = false;
    }

    return result;
  }

  // Find value from cache
  boost::optional<ValueType> find(const KeyType& key)
  {
    Lock lock(itsMutex);
    LeftIteratorType it = itsMap.left.find(key);

    if (it != itsMap.left.end())
    {
      // Check if any of the tags in the requested key have been phased out
      auto& tagSet = it->second.itsTagSet;
      for (auto tagIt = tagSet.begin(); tagIt != tagSet.end(); ++tagIt)
      {
        auto tagMapIt = itsTagMap.find(*tagIt);

        if (tagMapIt == itsTagMap.end())
        {
          // Tag has expired and tag map has been cleaned. Remove from cache
          itsMap.left.erase(it);
          FMI_CACHE_EXPIRED_DELETED;
          return boost::optional<ValueType>();
        }

        // If one of the tags is expired, remove object from cache
        if (FMI_CACHE_EXPIRATION_CALL(
                ExpirationPolicy::isExpired, tagMapIt->second.first, itsTimeConstant))
        {
          // This tag expired, erase the object from cache and return empty
          itsMap.left.erase(it);

          return boost::optional<ValueType>();
        }
      }

      EvictionPolicy<MapType, TagMapType, KeyType, ValueType, TagSetType, SizeFunc>::onAccess(
          itsMap, it);

      FMI_CACHE_HIT;

      // Update hit count for this entry
      ++it->second.itsHits;

      return boost::optional<ValueType>(it->second.itsValue);
    }

    else
    {
      FMI_CACHE_MISS;
      return boost::optional<ValueType>();
    }
  }

  // Find value from cache and return also its hits
  boost::optional<ValueType> find(const KeyType& key, std::size_t& hits)
  {
    Lock mapLock(itsMutex);
    LeftIteratorType it = itsMap.left.find(key);

    if (it != itsMap.left.end())
    {
      // Check if any of the tags in the requested key have been phased out
      auto& tagSet = it->second.itsTagSet;
      for (auto tagIt = tagSet.begin(); tagIt != tagSet.end(); ++tagIt)
      {
        auto tagMapIt = itsTagMap.find(*tagIt);

        if (tagMapIt == itsTagMap.end())
        {
          // Tag has expired and tag map has been cleaned. Remove from cache
          itsMap.left.erase(it);
          FMI_CACHE_EXPIRED_DELETED;
          return boost::optional<ValueType>();
        }

        // If one of the tags is expired, remove object from cache

        if (FMI_CACHE_EXPIRATION_CALL(
                ExpirationPolicy::isExpired, tagMapIt->second.first, itsTimeConstant))
        {
          // This tag expired, erase the object from cache and return empty
          itsMap.left.erase(it);

          return boost::optional<ValueType>();
        }
      }

      EvictionPolicy<MapType, TagMapType, KeyType, ValueType, TagSetType, SizeFunc>::onAccess(
          itsMap, it);

      FMI_CACHE_HIT;

      // Update hit count for this entry

      hits = ++it->second.itsHits;

      return boost::optional<ValueType>(it->second.itsValue);
    }

    else
    {
      FMI_CACHE_MISS;
      return boost::optional<ValueType>();
    }
  }

  void expire(const TagType& tagToExpire, const std::time_t& expirationTime = std::time(NULL))
  {
    Lock wlock(itsMutex);
    // Set data containing the given tag as expired
    auto tagIt = itsTagMap.find(tagToExpire);
    if (tagIt == itsTagMap.end())
    {
      // Trying to expire nonexisting tag
      return;
    }
    else
    {
      tagIt->second.first = expirationTime;
    }

    // If tag map has grown large, clean it up
    if (itsTagMap.size() >= itsMaxSize)
    {
      clearExpiredTags();
    }
  }

  void clear()
  {
    Lock lock(itsMutex);
    itsMap.clear();
    itsTagMap.clear();
    itsSize = 0;
  }

  // Resize cache
  void resize(std::size_t newMaxSize)
  {
    Lock lock(itsMutex);

    // Evict until the cache fits into new size
    itsMaxSize = newMaxSize;
    EvictionPolicy<MapType, TagMapType, KeyType, ValueType, TagSetType, SizeFunc>::onEvict(
        itsMap, itsTagMap, itsSize, itsMaxSize);
  }

  // Resize cache, return evicted items
  void resize(std::size_t newMaxSize, std::vector<std::pair<KeyType, ValueType> >& evictedItems)
  {
    evictedItems.clear();

    Lock lock(itsMutex);

    // Evict until the cache fits into new size
    itsMaxSize = newMaxSize;
    EvictionPolicy<MapType, TagMapType, KeyType, ValueType, TagSetType, SizeFunc>::onEvict(
        itsMap, itsTagMap, itsSize, itsMaxSize, evictedItems);
  }

  std::size_t size()
  {
    Lock lock(itsMutex);
    return itsSize;
  }

  std::size_t maxSize()
  {
    Lock lock(itsMutex);
    return itsMaxSize;
  }

  std::list<CacheReportingObjectType> getContent()
  {
    Lock lock(itsMutex);
    std::list<CacheReportingObjectType> result;
    for (RightIteratorType it = itsMap.right.begin(); it != itsMap.right.end(); ++it)
    {
      result.push_back(CacheReportingObjectType(it->second,
                                                it->first.itsValue,
                                                it->first.itsTagSet,
                                                it->first.itsHits,
                                                it->first.itsSize));
    }
    return result;
  }

#ifndef NDEBUG

  unsigned long getHits() { return cacheHits; }
  unsigned long getMisses() { return cacheMisses; }
  unsigned long getExpiredReturned() { return expiredReturned; }
  unsigned long getExpiredDeleted() { return expiredDeleted; }
  std::string getTextContent()
  {
    std::stringstream output;
    Lock lock(itsMutex);
    auto lastReal = itsMap.right.end();
    std::advance(lastReal, -1);
    for (auto it = itsMap.right.begin(); it != lastReal; ++it)
    {
      output << it->first.itsValue << ",";
    }
    output << lastReal->first.itsValue;

    return output.str();
  }

#endif

 private:
  // Clear expired tags from the tag map
  void clearExpiredTags()
  {
    for (auto tagIt = itsTagMap.begin(); tagIt != itsTagMap.end();)
    {
      if (ExpirationPolicy::toDelete(tagIt->second.first, itsTimeConstant))
      {
        itsTagMap.erase(tagIt++);
      }
      else
      {
        ++tagIt;
      }
    }
  }

  MapType itsMap;

  TagMapType itsTagMap;

  MutexType itsMutex;

  std::size_t itsSize;

  std::size_t itsMaxSize;

  long itsTimeConstant;

#ifndef NDEBUG
  // Debugging parameters
  unsigned long cacheHits;

  unsigned long cacheMisses;

  unsigned long expiredReturned;

  unsigned long expiredDeleted;
#endif
};

// Size_t parser for the FileCache
bool parse_size_t(const std::string& input, std::size_t& result);

// ----------------------------------------------------------------------
/*!
 * \brief File Cache object for filesystem-backed caching
 */
// ----------------------------------------------------------------------
namespace fs = boost::filesystem;

struct FileCacheStruct
{
  FileCacheStruct(const fs::path& thePath, std::size_t theSize) : path(thePath), fileSize(theSize)
  {
  }

  fs::path path;
  std::size_t fileSize;
};

class FileCache : boost::noncopyable
{
  typedef boost::shared_mutex MutexType;
  typedef boost::shared_lock<MutexType> ReadLock;
  typedef boost::unique_lock<MutexType> WriteLock;
  typedef boost::upgrade_lock<MutexType> UpgradeReadLock;
  typedef boost::upgrade_to_unique_lock<MutexType> UpgradeWriteLock;

  typedef boost::bimaps::bimap<boost::bimaps::unordered_set_of<std::size_t,
                                                               boost::hash<std::size_t>,
                                                               std::equal_to<std::size_t> >,
                               boost::bimaps::list_of<FileCacheStruct> > MapType;

 public:
  // ----------------------------------------------------------------------
  /*!
   * \brief Constructor
   * This attempts to validate the cache directory for permission failures etc.
   */
  // ----------------------------------------------------------------------
  FileCache(const fs::path& directory, std::size_t maxSize);

  // ----------------------------------------------------------------------
  /*!
   * \brief Find entry from the cache
   */
  // ----------------------------------------------------------------------

  boost::optional<std::string> find(std::size_t key);

  // ----------------------------------------------------------------------
  /*!
   * \brief Insert an entry into the cache.
   * If performCleanup is false, no cleanup is performed and if the entry
   * does not fit into the cache failure is returned.
   */
  // ----------------------------------------------------------------------

  bool insert(std::size_t key, const std::string& value, bool performCleanup = true);

  // ----------------------------------------------------------------------
  /*!
   * \brief Get keys from the cache
   */
  // ----------------------------------------------------------------------

  std::vector<std::size_t> getContent();

  // ----------------------------------------------------------------------
  /*!
   * \brief Get cache size in bytes
   */
  // ----------------------------------------------------------------------

  std::size_t getSize();
  // ----------------------------------------------------------------------
  /*!
   * \brief Do manual cleanup of the cache
   */
  // ----------------------------------------------------------------------

  bool clean(std::size_t spaceNeeded);

 private:
  // ----------------------------------------------------------------------
  /*!
   * \brief Performs cleanup
   */
  // ----------------------------------------------------------------------

  bool performCleanup(std::size_t space_needed);

  // ----------------------------------------------------------------------
  /*!
   * \brief Recurses through the cache directory structure and updates the cache map accordingly
   */
  // ----------------------------------------------------------------------

  void update();

  // ----------------------------------------------------------------------
  /*!
   * \brief Writes a file to disk
   */
  // ----------------------------------------------------------------------

  bool writeFile(const fs::path& theDir, const std::string& fileName, const std::string& theValue);
  // ----------------------------------------------------------------------
  /*!
   * \brief Checks that entry can be written to disk
   */
  // ----------------------------------------------------------------------

  bool checkForDiskSpace(const fs::path& thePath, const std::string& theValue, bool doCleanup);

  // ----------------------------------------------------------------------
  /*!
   * \brief Gets subdirectory and filename from hash value
   */
  // ----------------------------------------------------------------------

  std::pair<std::string, std::string> getFileDirAndName(std::size_t hashValue);

  // ----------------------------------------------------------------------
  /*!
   * \brief Gets hash value from subdirectory and filename
   */
  // ----------------------------------------------------------------------

  bool getKey(const std::string& directory, const std::string& filename, std::size_t& key);

  std::size_t itsSize;

  std::size_t itsMaxSize;

  fs::path itsDirectory;

  MapType itsContentMap;

  MutexType itsMutex;
};

}  // namespace Cache

}  // namespace Fmi
