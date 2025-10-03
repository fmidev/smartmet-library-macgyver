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

#include "LocalDateTime.h"
#include <string>

namespace Fmi
{
class DateTimeParser
{
 public:
  ~DateTimeParser();
  DateTimeParser();
  DateTimeParser(const DateTimeParser& other);
  DateTimeParser(DateTimeParser&& other) noexcept;
  DateTimeParser& operator=(const DateTimeParser& other);
  DateTimeParser& operator=(DateTimeParser&& other) noexcept;

  Fmi::LocalDateTime parse(const std::string& str,
                           const std::string& format,
                           const Fmi::TimeZonePtr& tz) const;
  Fmi::LocalDateTime parse(const std::string& str, const Fmi::TimeZonePtr& tz) const;

  DateTime parse(const std::string& str, const std::string& format) const;
  DateTime parse(const std::string& str) const;

  static DateTime parse_http(const std::string& str);

  TimeDuration parse_duration(const std::string& str) const;
  static TimeDuration parse_iso_duration(const std::string& str);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl;

};  // class DateTimeParser
}  // namespace Fmi

// ======================================================================
