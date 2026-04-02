// ======================================================================
/*!
 * \brief LRU caching with optional custom size function and cache striping
 */
// ======================================================================
#pragma once

// bimap kept for FileCache; Cache template uses unordered_map + list
#include <boost/bimap.hpp>
#include <boost/bimap/list_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/functional/hash.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <array>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <list>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <utility>

#include "CacheStats.h"
#include "DateTime.h"

namespace Fmi
{
namespace Cache
{

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
 * \brief Object stored in the cache (returned by getContent)
 */
// ----------------------------------------------------------------------

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
 * its own shared_mutex.  find() acquires an upgradeable shared lock and only
 * upgrades to exclusive when it needs to splice the LRU list (on hit), so
 * pure-read operations such as statistics() and size() can run concurrently
 * with in-progress finds.  insert(), upsert(), clear() and resize() take a
 * full exclusive lock on the affected shard.
 */
// ----------------------------------------------------------------------

template <class KeyType,
          class ValueType,
          class SizeFunc = TrivialSizeFunction<ValueType>,
          std::size_t NumShards = 16>
class Cache
{
 public:
  using CacheReportingObjectType = CacheReportingObject<KeyType, ValueType>;
  using ItemVector = std::vector<std::pair<KeyType, ValueType>>;

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
   * \brief Get cache statistics (shared lock per shard, non-blocking for finds)
   */
  // ----------------------------------------------------------------------
  CacheStats statistics() const
  {
    std::size_t totalSize = 0, totalInserts = 0, totalEvictions = 0;
    std::size_t totalHits = 0, totalMisses = 0;
    for (const auto& shard : itsShards)
    {
      boost::shared_lock<boost::shared_mutex> lock(shard.mutex);
      totalSize += shard.size;
      totalInserts += shard.insertCount;
      totalEvictions += shard.evictionCount;
      totalHits += shard.hitCount.load(std::memory_order_relaxed);
      totalMisses += shard.missCount.load(std::memory_order_relaxed);
    }
    return CacheStats(itsStartTime,
                      itsMaxSizePerShard * NumShards,
                      totalSize,
                      totalInserts,
                      totalHits,
                      totalMisses,
                      totalEvictions);
  }

  // Insert value; returns false if key already present or value exceeds shard capacity
  bool insert(const KeyType& key, const ValueType& value)
  {
    auto& shard = itsShards[getShardIndex(key)];
    boost::unique_lock<boost::shared_mutex> lock(shard.mutex);

    if (shard.map.count(key))
      return false;

    std::size_t valueSize = SizeFunc::getSize(value);
    if (valueSize > itsMaxSizePerShard)
      return false;

    shard.size += valueSize;
    const std::size_t mapSizeBefore = shard.map.size();
    evictLRU(shard);
    shard.evictionCount += mapSizeBefore - shard.map.size();

    shard.list.emplace_back(Entry{key, value, 0, valueSize});
    shard.map.emplace(key, std::prev(shard.list.end()));
    ++shard.insertCount;
    return true;
  }

  // Insert value; fills evictedItems with any entries that were displaced
  bool insert(const KeyType& key, const ValueType& value, ItemVector& evictedItems)
  {
    evictedItems.clear();

    auto& shard = itsShards[getShardIndex(key)];
    boost::unique_lock<boost::shared_mutex> lock(shard.mutex);

    if (shard.map.count(key))
      return false;

    std::size_t valueSize = SizeFunc::getSize(value);
    if (valueSize > itsMaxSizePerShard)
      return false;

    shard.size += valueSize;
    evictLRU(shard, evictedItems);
    shard.evictionCount += evictedItems.size();

    shard.list.emplace_back(Entry{key, value, 0, valueSize});
    shard.map.emplace(key, std::prev(shard.list.end()));
    ++shard.insertCount;
    return true;
  }

  // Upsert: insert or replace existing entry. Always counts as an insert.
  // Returns false only if value exceeds shard capacity.
  bool upsert(const KeyType& key, const ValueType& value)
  {
    auto& shard = itsShards[getShardIndex(key)];
    boost::unique_lock<boost::shared_mutex> lock(shard.mutex);

    // Remove existing entry without counting it as an eviction
    auto mapIt = shard.map.find(key);
    if (mapIt != shard.map.end())
    {
      shard.size -= mapIt->second->size;
      shard.list.erase(mapIt->second);
      shard.map.erase(mapIt);
    }

    std::size_t valueSize = SizeFunc::getSize(value);
    if (valueSize > itsMaxSizePerShard)
      return false;

    shard.size += valueSize;
    const std::size_t mapSizeBefore = shard.map.size();
    evictLRU(shard);
    shard.evictionCount += mapSizeBefore - shard.map.size();

    shard.list.emplace_back(Entry{key, value, 0, valueSize});
    shard.map.emplace(key, std::prev(shard.list.end()));
    ++shard.insertCount;
    return true;
  }

  // Find value; returns empty optional on miss. Upgrades to exclusive lock
  // only on hit (to update LRU order).
  std::optional<ValueType> find(const KeyType& key)
  {
    auto& shard = itsShards[getShardIndex(key)];
    boost::upgrade_lock<boost::shared_mutex> lock(shard.mutex);

    auto mapIt = shard.map.find(key);
    if (mapIt == shard.map.end())
    {
      shard.missCount.fetch_add(1, std::memory_order_relaxed);
      return {};
    }

    boost::upgrade_to_unique_lock<boost::shared_mutex> wlock(lock);
    shard.list.splice(shard.list.end(), shard.list, mapIt->second);
    ++mapIt->second->hits;
    shard.hitCount.fetch_add(1, std::memory_order_relaxed);
    return mapIt->second->value;
  }

  // Find value and also return its hit count
  std::optional<ValueType> find(const KeyType& key, std::size_t& hits)
  {
    auto& shard = itsShards[getShardIndex(key)];
    boost::upgrade_lock<boost::shared_mutex> lock(shard.mutex);

    auto mapIt = shard.map.find(key);
    if (mapIt == shard.map.end())
    {
      shard.missCount.fetch_add(1, std::memory_order_relaxed);
      return {};
    }

    boost::upgrade_to_unique_lock<boost::shared_mutex> wlock(lock);
    shard.list.splice(shard.list.end(), shard.list, mapIt->second);
    hits = ++mapIt->second->hits;
    shard.hitCount.fetch_add(1, std::memory_order_relaxed);
    return mapIt->second->value;
  }

  void clear()
  {
    for (auto& shard : itsShards)
    {
      boost::unique_lock<boost::shared_mutex> lock(shard.mutex);
      shard.list.clear();
      shard.map.clear();
      shard.size = 0;
    }
  }

  void resize(std::size_t newMaxSize)
  {
    itsMaxSizePerShard = (newMaxSize + NumShards - 1) / NumShards;
    for (auto& shard : itsShards)
    {
      boost::unique_lock<boost::shared_mutex> lock(shard.mutex);
      const std::size_t sizeBefore = shard.map.size();
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
      boost::unique_lock<boost::shared_mutex> lock(shard.mutex);
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
      boost::shared_lock<boost::shared_mutex> lock(shard.mutex);
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
      boost::shared_lock<boost::shared_mutex> lock(shard.mutex);
      for (const auto& entry : shard.list)
        result.emplace_back(entry.key, entry.value, entry.hits, entry.size);
    }
    return result;
  }

  std::string getTextContent() const
  {
    std::stringstream output;
    bool first = true;
    for (const auto& shard : itsShards)
    {
      boost::shared_lock<boost::shared_mutex> lock(shard.mutex);
      for (const auto& entry : shard.list)
      {
        if (!first)
          output << ',';
        first = false;
        output << entry.value;
      }
    }
    return output.str();
  }

 private:
  struct Entry
  {
    KeyType key;
    ValueType value;
    std::size_t hits = 0;
    std::size_t size = 0;
  };

  using ListType = std::list<Entry>;
  using MapType =
      std::unordered_map<KeyType, typename ListType::iterator, boost::hash<KeyType>>;

  struct Shard
  {
    ListType list;  // front = LRU, back = MRU
    MapType map;
    mutable boost::shared_mutex mutex;
    std::size_t size = 0;
    std::size_t insertCount = 0;
    std::size_t evictionCount = 0;
    // hit/miss updated lock-free on the miss path (no upgrade needed)
    mutable std::atomic<std::size_t> hitCount{0};
    mutable std::atomic<std::size_t> missCount{0};
  };

  std::size_t getShardIndex(const KeyType& key) const
  {
    constexpr std::size_t prime = 2654435761ULL;
    return (boost::hash<KeyType>{}(key) * prime) % NumShards;
  }

  // Evict LRU entries until shard is within capacity (caller holds exclusive lock)
  void evictLRU(Shard& shard)
  {
    while (shard.size > itsMaxSizePerShard && !shard.list.empty())
    {
      shard.size -= shard.list.front().size;
      shard.map.erase(shard.list.front().key);
      shard.list.pop_front();
    }
  }

  void evictLRU(Shard& shard, ItemVector& evicted)
  {
    while (shard.size > itsMaxSizePerShard && !shard.list.empty())
    {
      auto& e = shard.list.front();
      evicted.emplace_back(e.key, e.value);
      shard.size -= e.size;
      shard.map.erase(e.key);
      shard.list.pop_front();
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
  FileCache(const std::filesystem::path& directory, std::size_t maxSize);

  FileCache(const FileCache& other) = delete;
  FileCache(FileCache&& other) = delete;
  FileCache& operator=(const FileCache& other) = delete;
  FileCache& operator=(FileCache&& other) = delete;

  std::optional<std::string> find(std::size_t key);
  bool insert(std::size_t key, const std::string& value, bool performCleanup = true);
  std::vector<std::size_t> getContent() const;
  std::size_t getSize() const;
  bool clean(std::size_t spaceNeeded);
  CacheStats statistics() const;

 private:
  bool performCleanup(std::size_t space_needed);
  void update();
  bool writeFile(const std::filesystem::path& theDir,
                 const std::string& fileName,
                 const std::string& theValue) const;
  bool checkForDiskSpace(const std::filesystem::path& thePath,
                         const std::string& theValue,
                         bool doCleanup);
  std::pair<std::string, std::string> getFileDirAndName(std::size_t hashValue) const;
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
