// ======================================================================
/*!
 * \brief Parse timestamps
 *
 * Known formats: iso, epoch, timestamp, sql, xml
 *
 * \deprecated
 *
 * This header is deprecated. Please use DateTimeParser instead, which
 * is faster when used in loops. This header will be removed once FMI
 * has ported all code to use DateTimeParser instead.
 *
 * Note that DateTimeParser reveals only a minimal set of methods.
 * Some private methods hidden behind an implementation pointer
 * may have to be made public once old code is ported.
 */
// ======================================================================

#pragma once

#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <string>

namespace Fmi
{
namespace TimeParser
{
// Generic parsers

boost::local_time::local_date_time parse(const std::string& str,
                                         const std::string& format,
                                         boost::local_time::time_zone_ptr tz);

boost::local_time::local_date_time parse(const std::string& str,
                                         boost::local_time::time_zone_ptr tz);

boost::posix_time::ptime parse(const std::string& str, const std::string& format);

boost::posix_time::ptime parse(const std::string& str);

// Individual format parsers

std::string looks(const std::string& str);
bool looks_utc(const std::string& str);

boost::posix_time::ptime try_parse_iso(const std::string& str, bool* isutc);
boost::posix_time::ptime try_parse_offset(const std::string& str);

boost::posix_time::ptime parse_iso(const std::string& str);
boost::posix_time::ptime parse_epoch(const std::string& str);
boost::posix_time::ptime parse_sql(const std::string& str);
boost::posix_time::ptime parse_fmi(const std::string& str);
boost::posix_time::ptime parse_offset(const std::string& str);

// Specialized parsers

boost::posix_time::ptime parse_http(const std::string& str);

// Local date time creator which handles DST changes nicely. Used internally
// and provided as a convenice function for external parsers

boost::local_time::local_date_time make_time(const boost::gregorian::date& day,
                                             const boost::posix_time::time_duration& duration,
                                             const boost::local_time::time_zone_ptr& zone);

boost::posix_time::time_duration try_parse_duration(const std::string& str);
boost::posix_time::time_duration try_parse_iso_duration(const std::string& str);

boost::posix_time::time_duration parse_duration(const std::string& str);
boost::posix_time::time_duration parse_iso_duration(const std::string& str);

}  // namespace TimeParser
}  // namespace Fmi

// ======================================================================
