// ======================================================================
/*!
 * \brief CLOCK (second chance) caching with optional custom size function and cache striping
 *
 * ClockCache has the same interface as Fmi::Cache::Cache so that the two can be
 * swapped for comparison. The difference is the eviction policy and its locking:
 *
 * Cache keeps an exact LRU order. A find() hit on an entry which is not already the
 * most recently used one must move the entry to the MRU position, which requires an
 * exclusive lock on the shard. With many threads and many keys most finds therefore
 * serialize on the shard lock.
 *
 * ClockCache approximates LRU with the CLOCK algorithm. The entries of a shard form a
 * ring with a "hand". A find() hit only increments the hit counter of the entry, which
 * doubles as the CLOCK reference bit: an entry counts as referenced if its hit count
 * has changed since the hand last passed it. find() never takes an exclusive lock.
 *
 * On eviction the hand sweeps the ring: a referenced entry gets a second chance (its
 * current hit count is recorded and the hand moves on), an unreferenced entry is
 * evicted. New entries are placed just behind the hand, so they survive almost a full
 * revolution of the hand before they are examined for the first time. An entry which
 * is never found again is evicted the first time the hand reaches it, which makes the
 * cache more resistant to one-off lookups than LRU.
 *
 * Readers still write shared memory: the shard lock word (std::shared_mutex), the hit
 * counter of the entry and the hit counter of the shard. These are the same writes
 * Cache does on a hit which needs no LRU promotion.
 */
// ======================================================================
#pragma once

#include "Cache.h"
#include "CacheStats.h"
#include "DateTime.h"
#include <boost/functional/hash.hpp>
#include <array>
#include <atomic>
#include <list>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Fmi
{
namespace Cache
{

template <class KeyType,
          class ValueType,
          class SizeFunc = TrivialSizeFunction<ValueType>,
          std::size_t NumShards = 16>
class ClockCache
{
 public:
  using CacheReportingObjectType = CacheReportingObject<KeyType, ValueType>;
  using ItemVector = std::vector<std::pair<KeyType, ValueType>>;

  // Default constructor eases the use as data member
  ClockCache() = default;

  ClockCache(const ClockCache& other) = delete;
  ClockCache(ClockCache&& other) = delete;
  ClockCache& operator=(const ClockCache& other) = delete;
  ClockCache& operator=(ClockCache&& other) = delete;

  explicit ClockCache(std::size_t maxSize)
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
      std::shared_lock<std::shared_mutex> lock(shard.mutex);
      totalSize += shard.size;
      totalInserts += shard.insertCount;
      totalEvictions += shard.evictionCount;
      totalHits += shard.hitCount.load(std::memory_order_relaxed);
      totalMisses += shard.missCount.load(std::memory_order_relaxed);
    }
    return CacheStats(itsStartTime,
                      itsMaxSizePerShard.load(std::memory_order_relaxed) * NumShards,
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
    std::unique_lock<std::shared_mutex> lock(shard.mutex);

    if (shard.map.count(key))
      return false;

    std::size_t valueSize = SizeFunc::getSize(value);
    if (valueSize > itsMaxSizePerShard.load(std::memory_order_relaxed))
      return false;

    addEntry(shard, key, value, valueSize, nullptr);
    return true;
  }

  // Insert value; fills evictedItems with any entries that were displaced
  bool insert(const KeyType& key, const ValueType& value, ItemVector& evictedItems)
  {
    evictedItems.clear();

    auto& shard = itsShards[getShardIndex(key)];
    std::unique_lock<std::shared_mutex> lock(shard.mutex);

    if (shard.map.count(key))
      return false;

    std::size_t valueSize = SizeFunc::getSize(value);
    if (valueSize > itsMaxSizePerShard.load(std::memory_order_relaxed))
      return false;

    addEntry(shard, key, value, valueSize, &evictedItems);
    return true;
  }

  // Upsert: insert or replace existing entry. Always counts as an insert.
  // Returns false only if value exceeds shard capacity.
  bool upsert(const KeyType& key, const ValueType& value)
  {
    auto& shard = itsShards[getShardIndex(key)];
    std::unique_lock<std::shared_mutex> lock(shard.mutex);

    // Remove existing entry without counting it as an eviction
    auto mapIt = shard.map.find(key);
    if (mapIt != shard.map.end())
      removeEntry(shard, mapIt);

    std::size_t valueSize = SizeFunc::getSize(value);
    if (valueSize > itsMaxSizePerShard.load(std::memory_order_relaxed))
      return false;

    addEntry(shard, key, value, valueSize, nullptr);
    return true;
  }

  // Find value; returns empty optional on miss.
  std::optional<ValueType> find(const KeyType& key)
  {
    std::size_t hits = 0;
    return find(key, hits);
  }

  // Find value and also return its hit count. Only a shared lock is taken, the hit
  // counter of the entry serves as the CLOCK reference bit.
  std::optional<ValueType> find(const KeyType& key, std::size_t& hits)
  {
    auto& shard = itsShards[getShardIndex(key)];
    std::shared_lock<std::shared_mutex> lock(shard.mutex);

    auto mapIt = shard.map.find(key);
    if (mapIt == shard.map.end())
    {
      shard.missCount.fetch_add(1, std::memory_order_relaxed);
      return {};
    }

    const auto& entry = *mapIt->second;
    hits = entry.hits.fetch_add(1, std::memory_order_relaxed) + 1;
    shard.hitCount.fetch_add(1, std::memory_order_relaxed);
    return entry.value;
  }

  void clear()
  {
    for (auto& shard : itsShards)
    {
      std::unique_lock<std::shared_mutex> lock(shard.mutex);
      shard.map.clear();
      shard.list.clear();
      shard.hand = shard.list.end();
      shard.size = 0;
    }
  }

  void resize(std::size_t newMaxSize)
  {
    itsMaxSizePerShard.store((newMaxSize + NumShards - 1) / NumShards, std::memory_order_relaxed);
    for (auto& shard : itsShards)
    {
      std::unique_lock<std::shared_mutex> lock(shard.mutex);
      evict(shard, nullptr);
    }
  }

  void resize(std::size_t newMaxSize, ItemVector& evictedItems)
  {
    evictedItems.clear();
    itsMaxSizePerShard.store((newMaxSize + NumShards - 1) / NumShards, std::memory_order_relaxed);
    for (auto& shard : itsShards)
    {
      std::unique_lock<std::shared_mutex> lock(shard.mutex);
      evict(shard, &evictedItems);
    }
  }

  std::size_t size() const
  {
    std::size_t total = 0;
    for (const auto& shard : itsShards)
    {
      std::shared_lock<std::shared_mutex> lock(shard.mutex);
      total += shard.size;
    }
    return total;
  }

  std::size_t maxSize() const
  {
    return itsMaxSizePerShard.load(std::memory_order_relaxed) * NumShards;
  }

  // The entries of each shard are listed in the order the hand will examine them,
  // i.e. the next eviction candidate first.
  std::list<CacheReportingObjectType> getContent() const
  {
    std::list<CacheReportingObjectType> result;
    for (const auto& shard : itsShards)
    {
      std::shared_lock<std::shared_mutex> lock(shard.mutex);
      forEachInClockOrder(shard,
                          [&result](const Entry& entry)
                          {
                            result.emplace_back(entry.key,
                                                entry.value,
                                                entry.hits.load(std::memory_order_relaxed),
                                                entry.size);
                          });
    }
    return result;
  }

  std::string getTextContent() const
  {
    std::stringstream output;
    bool first = true;
    for (const auto& shard : itsShards)
    {
      std::shared_lock<std::shared_mutex> lock(shard.mutex);
      forEachInClockOrder(shard,
                          [&output, &first](const Entry& entry)
                          {
                            if (!first)
                              output << ',';
                            first = false;
                            output << entry.value;
                          });
    }
    return output.str();
  }

 private:
  struct Entry
  {
    Entry(KeyType k, ValueType v, std::size_t s) : key(std::move(k)), value(std::move(v)), size(s)
    {
    }

    KeyType key;
    ValueType value;
    mutable std::atomic<std::size_t> hits{0};  // updated by find() under a shared lock
    std::size_t size = 0;
    std::size_t seenHits = 0;  // hits when the hand last passed, accessed under exclusive lock
  };

  using ListType = std::list<Entry>;
  using MapType = std::unordered_map<KeyType, typename ListType::iterator, boost::hash<KeyType>>;

  struct Shard
  {
    ListType list;  // the ring, end() wraps to begin()
    typename ListType::iterator hand = list.end();
    MapType map;
    mutable std::shared_mutex mutex;
    std::size_t size = 0;
    std::size_t insertCount = 0;
    std::size_t evictionCount = 0;
    // hit/miss counters are updated under a shared lock, hence atomic
    mutable std::atomic<std::size_t> hitCount{0};
    mutable std::atomic<std::size_t> missCount{0};
  };

  std::size_t getShardIndex(const KeyType& key) const
  {
    constexpr std::size_t prime = 2654435761ULL;
    return (boost::hash<KeyType>{}(key)*prime) % NumShards;
  }

  // Make room for and add a new entry just behind the hand (caller holds exclusive lock)
  void addEntry(Shard& shard,
                const KeyType& key,
                const ValueType& value,
                std::size_t valueSize,
                ItemVector* evictedItems)
  {
    shard.size += valueSize;
    evict(shard, evictedItems);

    auto it = shard.list.emplace(shard.hand, key, value, valueSize);
    shard.map.emplace(key, it);
    ++shard.insertCount;
  }

  // Remove an entry without counting it as an eviction (caller holds exclusive lock)
  void removeEntry(Shard& shard, typename MapType::iterator mapIt)
  {
    auto listIt = mapIt->second;
    if (shard.hand == listIt)
      ++shard.hand;
    shard.size -= listIt->size;
    shard.map.erase(mapIt);
    shard.list.erase(listIt);
  }

  // Sweep the hand until the shard is within capacity (caller holds exclusive lock).
  // Referenced entries get a second chance, so the loop ends within two revolutions.
  // The limit is loaded once so a concurrent resize cannot change it mid-pass.
  void evict(Shard& shard, ItemVector* evictedItems)
  {
    const std::size_t limit = itsMaxSizePerShard.load(std::memory_order_relaxed);
    while (shard.size > limit && !shard.list.empty())
    {
      if (shard.hand == shard.list.end())
        shard.hand = shard.list.begin();

      Entry& entry = *shard.hand;
      const std::size_t hits = entry.hits.load(std::memory_order_relaxed);
      if (hits != entry.seenHits)
      {
        entry.seenHits = hits;
        ++shard.hand;
        continue;
      }

      if (evictedItems != nullptr)
        evictedItems->emplace_back(entry.key, entry.value);
      shard.size -= entry.size;
      shard.map.erase(entry.key);
      shard.hand = shard.list.erase(shard.hand);
      ++shard.evictionCount;
    }
  }

  // Visit the entries starting from the hand (caller holds at least a shared lock)
  template <typename Visitor>
  static void forEachInClockOrder(const Shard& shard, Visitor visitor)
  {
    typename ListType::const_iterator start = shard.hand;
    for (auto it = start; it != shard.list.end(); ++it)
      visitor(*it);
    for (auto it = shard.list.begin(); it != start; ++it)
      visitor(*it);
  }

  std::array<Shard, NumShards> itsShards;
  // Atomic since it is the only cache-wide state: resize() writes it while
  // readers hold at most a shard lock, which cannot synchronize the access
  std::atomic<std::size_t> itsMaxSizePerShard{0};
  const DateTime itsStartTime = Fmi::SecondClock::universal_time();
};

}  // namespace Cache
}  // namespace Fmi
