// ======================================================================
/*!
 * \brief LRU caching with optional custom size function and cache striping
 */
// ======================================================================
#pragma once

#include <boost/bimap.hpp>
#include <boost/bimap/list_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/functional/hash.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/thread.hpp>
#include <array>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <sstream>
#include <utility>

#include "CacheStats.h"
#include "DateTime.h"

namespace Fmi
{
namespace Cache
{
using MutexType = boost::mutex;
using Lock = boost::lock_guard<MutexType>;

const CacheStats EMPTY_CACHE_STATS;

// ----------------------------------------------------------------------
/*!
 * \brief Size function for trivial (count-based) cache capacity
 */
// ----------------------------------------------------------------------

template <class ValueType>
struct TrivialSizeFunction
{
  static std::size_t getSize(const ValueType& /* theValue */) { return 1; }
};

// ----------------------------------------------------------------------
/*!
 * \brief Object stored in the cache
 */
// ----------------------------------------------------------------------

template <class KeyType, class ValueType>
struct CacheObject
{
  CacheObject(const KeyType& theKey, const ValueType& theValue, std::size_t theSize)
      : itsKey(theKey), itsValue(theValue), itsSize(theSize)
  {
  }

  KeyType itsKey;
  ValueType itsValue;
  std::size_t itsHits = 0;
  std::size_t itsSize = 0;
};

template <class KeyType, class ValueType>
struct CacheReportingObject
{
  CacheReportingObject(const KeyType& theKey,
                       const ValueType& theValue,
                       std::size_t theHits,
                       std::size_t theSize)
      : itsKey(theKey), itsValue(theValue), itsHits(theHits), itsSize(theSize)
  {
  }

  KeyType itsKey;
  ValueType itsValue;
  std::size_t itsHits = 0;
  std::size_t itsSize = 0;
};

// ----------------------------------------------------------------------
/*!
 * \brief LRU cache with cache striping to reduce lock contention
 *
 * Keys are distributed across NumShards independent sub-caches, each with
 * its own mutex. Concurrent operations on different keys only contend when
 * they hash to the same shard.
 */
// ----------------------------------------------------------------------

template <class KeyType,
          class ValueType,
          class SizeFunc = TrivialSizeFunction<ValueType>,
          std::size_t NumShards = 16>
class Cache
{
 public:
  using CacheObjectType = CacheObject<KeyType, ValueType>;
  using CacheReportingObjectType = CacheReportingObject<KeyType, ValueType>;

  using MapType = boost::bimaps::bimap<
      boost::bimaps::unordered_set_of<KeyType, boost::hash<KeyType>, std::equal_to<KeyType> >,
      boost::bimaps::list_of<CacheObjectType> >;

  using LeftIteratorType = typename MapType::left_iterator;
  using ItemVector = std::vector<std::pair<KeyType, ValueType> >;

  // Default constructor eases the use as data member
  Cache() = default;

  Cache(const Cache& other) = delete;
  Cache(Cache&& other) = delete;
  Cache& operator=(const Cache& other) = delete;
  Cache& operator=(Cache&& other) = delete;

  explicit Cache(std::size_t maxSize)
      : itsMaxSizePerShard((maxSize + NumShards - 1) / NumShards)
  {
    static_assert(NumShards > 0, "NumShards must be greater than 0");
  }

  // ----------------------------------------------------------------------
  /*!
   * \brief Get cache statistics
   */
  // ----------------------------------------------------------------------
  CacheStats statistics() const
  {
    std::size_t totalSize = 0, totalInserts = 0, totalHits = 0, totalMisses = 0,
                totalEvictions = 0;
    for (const auto& shard : itsShards)
    {
      Lock lock(shard.mutex);
      totalSize += shard.size;
      totalInserts += shard.insertCount;
      totalHits += shard.hitCount;
      totalMisses += shard.missCount;
      totalEvictions += shard.evictionCount;
    }
    return CacheStats(itsStartTime,
                      itsMaxSizePerShard * NumShards,
                      totalSize,
                      totalInserts,
                      totalHits,
                      totalMisses,
                      totalEvictions);
  }

  // Insert value, returns true on success (false if already present or too large)
  bool insert(const KeyType& key, const ValueType& value)
  {
    auto& shard = itsShards[getShardIndex(key)];
    Lock lock(shard.mutex);

    if (shard.map.left.find(key) != shard.map.left.end())
      return false;

    std::size_t valueSize = SizeFunc::getSize(value);
    if (valueSize > itsMaxSizePerShard)
      return false;

    std::size_t sizeBefore = shard.map.size();
    shard.size += valueSize;
    evictLRU(shard);

    shard.map.insert(typename MapType::value_type(key, CacheObjectType(key, value, valueSize)));
    ++shard.insertCount;
    shard.evictionCount += sizeBefore + 1 - shard.map.size();
    return true;
  }

  // Insert value, returns true on success; fills evictedItems with any evicted entries
  bool insert(const KeyType& key, const ValueType& value, ItemVector& evictedItems)
  {
    evictedItems.clear();

    auto& shard = itsShards[getShardIndex(key)];
    Lock lock(shard.mutex);

    if (shard.map.left.find(key) != shard.map.left.end())
      return false;

    std::size_t valueSize = SizeFunc::getSize(value);
    if (valueSize > itsMaxSizePerShard)
      return false;

    shard.size += valueSize;
    evictLRU(shard, evictedItems);

    shard.map.insert(typename MapType::value_type(key, CacheObjectType(key, value, valueSize)));
    ++shard.insertCount;
    shard.evictionCount += evictedItems.size();
    return true;
  }

  // Find value; returns empty optional on miss
  std::optional<ValueType> find(const KeyType& key)
  {
    auto& shard = itsShards[getShardIndex(key)];
    Lock lock(shard.mutex);

    LeftIteratorType it = shard.map.left.find(key);
    if (it == shard.map.left.end())
    {
      ++shard.missCount;
      return {};
    }

    // Move to MRU position
    shard.map.right.relocate(shard.map.right.end(), shard.map.project_right(it));
    ++shard.hitCount;
    ++it->second.itsHits;
    return it->second.itsValue;
  }

  // Find value and also return its hit count
  std::optional<ValueType> find(const KeyType& key, std::size_t& hits)
  {
    auto& shard = itsShards[getShardIndex(key)];
    Lock lock(shard.mutex);

    LeftIteratorType it = shard.map.left.find(key);
    if (it == shard.map.left.end())
    {
      ++shard.missCount;
      return {};
    }

    shard.map.right.relocate(shard.map.right.end(), shard.map.project_right(it));
    ++shard.hitCount;
    hits = ++it->second.itsHits;
    return it->second.itsValue;
  }

  void clear()
  {
    for (auto& shard : itsShards)
    {
      Lock lock(shard.mutex);
      shard.map.clear();
      shard.size = 0;
    }
  }

  void resize(std::size_t newMaxSize)
  {
    itsMaxSizePerShard = (newMaxSize + NumShards - 1) / NumShards;
    for (auto& shard : itsShards)
    {
      Lock lock(shard.mutex);
      std::size_t sizeBefore = shard.map.size();
      evictLRU(shard);
      shard.evictionCount += sizeBefore - shard.map.size();
    }
  }

  void resize(std::size_t newMaxSize, ItemVector& evictedItems)
  {
    evictedItems.clear();
    itsMaxSizePerShard = (newMaxSize + NumShards - 1) / NumShards;
    for (auto& shard : itsShards)
    {
      ItemVector shardEvicted;
      Lock lock(shard.mutex);
      evictLRU(shard, shardEvicted);
      shard.evictionCount += shardEvicted.size();
      for (auto& item : shardEvicted)
        evictedItems.push_back(std::move(item));
    }
  }

  std::size_t size() const
  {
    std::size_t total = 0;
    for (const auto& shard : itsShards)
    {
      Lock lock(shard.mutex);
      total += shard.size;
    }
    return total;
  }

  std::size_t maxSize() const { return itsMaxSizePerShard * NumShards; }

  std::list<CacheReportingObjectType> getContent() const
  {
    std::list<CacheReportingObjectType> result;
    for (const auto& shard : itsShards)
    {
      Lock lock(shard.mutex);
      for (auto it = shard.map.right.begin(); it != shard.map.right.end(); ++it)
        result.push_back(CacheReportingObjectType(
            it->second, it->first.itsValue, it->first.itsHits, it->first.itsSize));
    }
    return result;
  }

  std::string getTextContent() const
  {
    std::stringstream output;
    bool first = true;
    for (const auto& shard : itsShards)
    {
      Lock lock(shard.mutex);
      for (auto it = shard.map.right.begin(); it != shard.map.right.end(); ++it)
      {
        if (!first)
          output << ',';
        first = false;
        output << it->first.itsValue;
      }
    }
    return output.str();
  }

 private:
  struct Shard
  {
    MapType map;
    mutable MutexType mutex;
    std::size_t size = 0;
    std::size_t insertCount = 0;
    std::size_t missCount = 0;
    std::size_t hitCount = 0;
    std::size_t evictionCount = 0;
  };

  std::size_t getShardIndex(const KeyType& key) const
  {
    constexpr std::size_t prime = 2654435761ULL;
    return (boost::hash<KeyType>{}(key) * prime) % NumShards;
  }

  // Evict LRU entries until shard is within capacity (caller holds shard lock)
  void evictLRU(Shard& shard)
  {
    while (shard.size > itsMaxSizePerShard && !shard.map.empty())
    {
      shard.size -= shard.map.right.front().first.itsSize;
      shard.map.right.pop_front();
    }
  }

  void evictLRU(Shard& shard, ItemVector& evicted)
  {
    while (shard.size > itsMaxSizePerShard && !shard.map.empty())
    {
      auto& item = shard.map.right.front().first;
      evicted.emplace_back(item.itsKey, item.itsValue);
      shard.size -= item.itsSize;
      shard.map.right.pop_front();
    }
  }

  std::array<Shard, NumShards> itsShards;
  std::size_t itsMaxSizePerShard = 0;
  const DateTime itsStartTime = Fmi::SecondClock::universal_time();
};

// Size_t parser for the FileCache
bool parse_size_t(const std::string& input, std::size_t& result);

// ----------------------------------------------------------------------
/*!
 * \brief File Cache object for filesystem-backed caching
 */
// ----------------------------------------------------------------------
struct FileCacheStruct
{
  FileCacheStruct(const std::filesystem::path& thePath, std::size_t theSize)
      : path(thePath), fileSize(theSize)
  {
  }

  std::filesystem::path path;
  std::size_t fileSize;
};

class FileCache
{
  using MutexType = boost::shared_mutex;
  using ReadLock = boost::shared_lock<MutexType>;
  using WriteLock = boost::unique_lock<MutexType>;
  using UpgradeReadLock = boost::upgrade_lock<MutexType>;
  using UpgradeWriteLock = boost::upgrade_to_unique_lock<MutexType>;

  using MapType = boost::bimaps::bimap<boost::bimaps::unordered_set_of<std::size_t,
                                                                       boost::hash<std::size_t>,
                                                                       std::equal_to<std::size_t> >,
                                       boost::bimaps::list_of<FileCacheStruct> >;

 public:
  // ----------------------------------------------------------------------
  /*!
   * \brief Constructor
   * This attempts to validate the cache directory for permission failures etc.
   */
  // ----------------------------------------------------------------------
  FileCache(const std::filesystem::path& directory, std::size_t maxSize);

  FileCache(const FileCache& other) = delete;
  FileCache(FileCache&& other) = delete;
  FileCache& operator=(const FileCache& other) = delete;
  FileCache& operator=(FileCache&& other) = delete;

  // ----------------------------------------------------------------------
  /*!
   * \brief Find entry from the cache
   */
  // ----------------------------------------------------------------------

  std::optional<std::string> find(std::size_t key);

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

  std::vector<std::size_t> getContent() const;

  // ----------------------------------------------------------------------
  /*!
   * \brief Get cache size in bytes
   */
  // ----------------------------------------------------------------------

  std::size_t getSize() const;

  // ----------------------------------------------------------------------
  /*!
   * \brief Do manual cleanup of the cache
   */
  // ----------------------------------------------------------------------

  bool clean(std::size_t spaceNeeded);

  // ----------------------------------------------------------------------
  /*!
   * \brief Get cache statistics
   */
  // ----------------------------------------------------------------------

  CacheStats statistics() const;

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

  bool writeFile(const std::filesystem::path& theDir,
                 const std::string& fileName,
                 const std::string& theValue) const;

  // ----------------------------------------------------------------------
  /*!
   * \brief Checks that entry can be written to disk
   */
  // ----------------------------------------------------------------------

  bool checkForDiskSpace(const std::filesystem::path& thePath,
                         const std::string& theValue,
                         bool doCleanup);

  // ----------------------------------------------------------------------
  /*!
   * \brief Gets subdirectory and filename from hash value
   */
  // ----------------------------------------------------------------------

  std::pair<std::string, std::string> getFileDirAndName(std::size_t hashValue) const;

  // ----------------------------------------------------------------------
  /*!
   * \brief Gets hash value from subdirectory and filename
   */
  // ----------------------------------------------------------------------

  bool getKey(const std::string& directory, const std::string& filename, std::size_t& key) const;

  std::size_t itsSize = 0;

  std::size_t itsMaxSize = 0;

  std::size_t itsInsertCount = 0;
  std::size_t itsMissCount = 0;
  std::size_t itsHitCount = 0;
  std::size_t itsEvictionCount = 0;

  const DateTime itsStartTime = Fmi::SecondClock::universal_time();

  std::filesystem::path itsDirectory;

  MapType itsContentMap;

  mutable MutexType itsMutex;
};

}  // namespace Cache
}  // namespace Fmi
