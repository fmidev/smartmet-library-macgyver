// ======================================================================
/*!
 * \brief Interface of class CacheStats
 */
// ======================================================================

#pragma once

#include <boost/date_time/posix_time/ptime.hpp>
#include <map>

namespace Fmi
{
namespace Cache
{
struct CacheStats
{
  CacheStats() = default;
  CacheStats(const boost::posix_time::ptime& t) : starttime(t) {}
  CacheStats(const boost::posix_time::ptime& t,
             std::size_t maxsize_,
             std::size_t size_,
             std::size_t inserts_,
             std::size_t hits_,
             std::size_t misses_)
      : starttime(t),
        maxsize(maxsize_),
        size(size_),
        inserts(inserts_),
        hits(hits_),
        misses(misses_)
  {
  }

  boost::posix_time::ptime starttime;
  std::size_t maxsize = 0;
  std::size_t size = 0;
  std::size_t inserts = 0;
  std::size_t hits = 0;
  std::size_t misses = 0;
};

using CacheStatistics = std::map<std::string, CacheStats>;

}  // namespace Cache
}  // namespace Fmi
