// ======================================================================
/*!
 * \brief Interface of class CacheStats
 */
// ======================================================================

#pragma once

#include <boost/date_time/posix_time/ptime.hpp>
#include <atomic>
#include <map>

namespace Fmi
{
namespace Cache
{

class CacheStats
{
public:
  CacheStats() {}
 CacheStats(size_t hits, size_t misses, const boost::posix_time::ptime& t): mHits(hits), mMisses(misses), mStartTime(t) {}
  CacheStats(const CacheStats& stats)
	: mHits(stats.hits()) ,mMisses(stats.misses()) ,mStartTime(stats.startTime()) {}
  void hit() { ++mHits; }
  void miss() { ++mMisses; }
  std::size_t hits() const { return mHits; }
  std::size_t misses() const { return mMisses; }
  const boost::posix_time::ptime& startTime() const { return mStartTime; }
private:
  std::atomic_size_t mHits{0};
  std::atomic_size_t mMisses{0};
  const boost::posix_time::ptime mStartTime{boost::posix_time::second_clock::universal_time()};
};

using CacheStatistics = std::map<std::string, CacheStats>;

}  // namespace Cache
}  // namespace Fmi

// ======================================================================
