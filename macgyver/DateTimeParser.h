// ======================================================================
/*!
 * \brief Parse timestamps
 *
 * This replaces the TimeParser namespace, which is not efficient when
 * parsing large amounts of timestamps such as when parsing database
 * output.
 *
 * Known time formats: iso, epoch, timestamp, sql, xml
 * Known duration formats: iso, FMI custom
 */
// ======================================================================

#pragma once

#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/noncopyable.hpp>
#include <string>

namespace Fmi
{
class DateTimeParser : private boost::noncopyable
{
 public:
  ~DateTimeParser();
  DateTimeParser();

  boost::local_time::local_date_time parse(const std::string& str,
                                           const std::string& format,
                                           boost::local_time::time_zone_ptr tz) const;
  boost::local_time::local_date_time parse(const std::string& str,
                                           boost::local_time::time_zone_ptr tz) const;

  boost::posix_time::ptime parse(const std::string& str, const std::string& format) const;
  boost::posix_time::ptime parse(const std::string& str) const;

  boost::posix_time::ptime parse_http(const std::string& str) const;

  boost::posix_time::time_duration parse_duration(const std::string& str) const;
  boost::posix_time::time_duration parse_iso_duration(const std::string& str) const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl;

};  // class DateTimeParser
}  // namespace Fmi

// ======================================================================
