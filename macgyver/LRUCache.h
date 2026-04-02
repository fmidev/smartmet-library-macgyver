// Thin wrapper around Fmi::Cache::Cache providing the LRUCache API
// (put/get/getStats) for backward compatibility with existing call sites.

#pragma once

#include "Cache.h"
#include <cstddef>
#include <memory>
#include <optional>

namespace Fmi
{
template <typename T, std::size_t NumShards = 32>
class LRUCache
{
  using ValueType = std::shared_ptr<T>;
  Fmi::Cache::Cache<std::size_t, ValueType, Fmi::Cache::TrivialSizeFunction<ValueType>, NumShards>
      itsCache;

 public:
  explicit LRUCache(std::size_t total_capacity) : itsCache(total_capacity) {}

  void put(std::size_t key, const ValueType& value) { itsCache.upsert(key, value); }

  std::optional<ValueType> get(std::size_t key) { return itsCache.find(key); }

  Fmi::Cache::CacheStats getStats() const { return itsCache.statistics(); }

  std::size_t getTotalCapacity() const { return itsCache.maxSize(); }

  std::size_t getCapacityPerShard() const { return itsCache.maxSize() / NumShards; }
};

}  // namespace Fmi
