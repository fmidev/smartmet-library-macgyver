// ======================================================================
/*!
 * \brief Format timestamps
 *
 * Known formats: iso, epoch, timestamp, sql, xml
 */
// ======================================================================

#pragma once

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_date_time.hpp>
#include <string>

namespace Fmi
{
class TimeFormatter
{
 public:
  static TimeFormatter* create(const std::string& name);

  virtual ~TimeFormatter();

  virtual std::string format(const boost::posix_time::ptime& t) const = 0;

  virtual std::string format(const boost::local_time::local_date_time& t) const = 0;
};
}

// ======================================================================
