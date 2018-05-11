// ======================================================================
/*!
 * \brief Parse timestamps
 */
// ======================================================================

#include "DateTimeParser.h"
#include "TimeParserDefinitions.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/regex.hpp>
#include <cctype>
#include <stdexcept>

using namespace Fmi::TimeParser;

namespace
{
boost::posix_time::ptime bad_time{boost::posix_time::not_a_date_time};
boost::posix_time::time_duration bad_duration{boost::posix_time::not_a_date_time};

boost::regex iso8601_weeks{"^P(\\d+)W$"};
boost::regex iso8601_short{"^P([[:d:]]+Y)?([[:d:]]+M)?([[:d:]]+D)?$"};
#if 0
boost::regex iso8601_long{
    "^P([[:d:]]+Y)?([[:d:]]+M)?([[:d:]]+D)?T([[:d:]]+H)?([[:d:]]+M)?([[:d:]]+S|[[:d:]]+\\.[[:d:]]+"
    "S)?$"};
#endif

boost::regex iso8601_long{
    "^P([[:d:]]+Y)?([[:d:]]+M)?([[:d:]]+D)?(T([[:d:]]+H)?([[:d:]]+M)?([[:d:]]+S|[[:d:]]+\\.[[:d:]]+"
    "S)?)?$"};

boost::posix_time::ptime buildFromSQL(const TimeStamp& target)
{
  unsigned int hour = 0, minute = 0, second = 0;
  if (target.hour) hour = *target.hour;
  if (target.minute) minute = *target.minute;
  if (target.second) second = *target.second;

  boost::posix_time::ptime ret;

  // Translate the exception to runtime_error
  try
  {
    ret = boost::posix_time::ptime(boost::gregorian::date(target.year, target.month, target.day),
                                   boost::posix_time::hours(hour) +
                                       boost::posix_time::minutes(minute) +
                                       boost::posix_time::seconds(second));
  }
  catch (const std::exception& err)
  {
    throw std::runtime_error(err.what());
  }

  return ret;
}

boost::posix_time::ptime buildFromISO(const TimeStamp& target)
{
  unsigned int hour = 0, minute = 0, second = 0;

  if (target.hour) hour = *target.hour;
  if (target.minute) minute = *target.minute;
  if (target.second) second = *target.second;

  boost::posix_time::ptime res;

  try
  {
    res = boost::posix_time::ptime(boost::gregorian::date(target.year, target.month, target.day),
                                   boost::posix_time::hours(hour) +
                                       boost::posix_time::minutes(minute) +
                                       boost::posix_time::seconds(second));
  }
  catch (const std::exception& err)
  {
    throw std::runtime_error(err.what());
  }

  // Do timezone
  // Sign is parsed separately to avoid mixing unsigned and
  // signed values in hour and minute definitions. The sign
  // must be parsed exactly once, not separately for hour and minute
  if (target.tz.sign == '+')
  {
    res -= boost::posix_time::hours(target.tz.hours);
    res -= boost::posix_time::minutes(target.tz.minutes);
  }
  else
  {
    res += boost::posix_time::hours(target.tz.hours);
    res += boost::posix_time::minutes(target.tz.minutes);
  }

  return res;
}

boost::posix_time::ptime buildFromEpoch(const UnixTime& target)
{
  return boost::posix_time::from_time_t(target);
}

boost::posix_time::ptime buildFromOffset(boost::posix_time::time_duration offset)
{
  // Apply to current time rounded to closest minute

  boost::posix_time::ptime now = boost::posix_time::second_clock::universal_time();
  boost::posix_time::time_duration tnow = now.time_of_day();
  int secs = tnow.seconds();

  if (secs >= 30)
    offset += boost::posix_time::seconds(60 - secs);  // round up
  else
    offset -= boost::posix_time::seconds(secs);  // round down

  // Construct the shifted time

  return boost::posix_time::ptime(now.date(), tnow + offset);
}

unsigned short get_short_month(const std::string& str)
{
  if (str == "Jan") return 1;
  if (str == "Feb") return 2;
  if (str == "Mar") return 3;
  if (str == "Apr") return 4;
  if (str == "May") return 5;
  if (str == "Jun") return 6;
  if (str == "Jul") return 7;
  if (str == "Aug") return 8;
  if (str == "Sep") return 9;
  if (str == "Oct") return 10;
  if (str == "Nov") return 11;
  if (str == "Dec") return 12;
  throw std::runtime_error("Invalid month name '" + str + "'");
}

bool is_short_month(const std::string& str) { return (get_short_month(str) > 0); }

bool is_short_weekday(const std::string& str)
{
  return (str == "Sun" || str == "Mon" || str == "Tue" || str == "Wed" || str == "Thu" ||
          str == "Fri" || str == "Sat");
}

bool is_long_weekday(const std::string& str)
{
  return (str == "Sunday" || str == "Monday" || str == "Tuesday" || str == "Wednesday" ||
          str == "Thursday" || str == "Friday" || str == "Saturday");
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse an unsigned integer from a C-string
 */
// ----------------------------------------------------------------------

bool parse_ushort(const char** str, unsigned int length, unsigned short* value)
{
  const char* ptr = *str;

  unsigned short tmp = 0;
  for (unsigned int i = 0; i < length; i++)
  {
    if (!isdigit(*ptr)) return false;
    tmp *= 10;
    tmp += static_cast<unsigned int>(*ptr - '0');
    ++ptr;
  }
  *value = tmp;
  *str = ptr;
  return true;
}

// ----------------------------------------------------------------------
/*!
 * \brief ISO 8601 dates use "-" as a date separator and ":" as a time separator
 */
// ----------------------------------------------------------------------

bool skip_separator(const char** str, char separator, bool extended_format)
{
  if (!extended_format)
    return true;
  else if (**str != separator)
    return false;
  ++*str;
  return true;
}

// ----------------------------------------------------------------------
/*!
 * \brief Return true if string looks like a nonnegative integer
 */
// ----------------------------------------------------------------------

bool looks_integer(const std::string& str)
{
  return boost::algorithm::all(str, boost::algorithm::is_digit());
}

// ----------------------------------------------------------------------
/*!
 * \brief Test if string looks like an SQL timestamp
 *
 * Format: YYYY-MM-DD HH:MI:SS
 */
// ----------------------------------------------------------------------

bool looks_sql(const std::string& t)
{
  return (t.size() == 19 && t[4] == '-' && t[7] == '-' && t[10] == ' ' && t[13] == ':' &&
          t[16] == ':' && looks_integer(t.substr(0, 4)) && looks_integer(t.substr(5, 2)) &&
          looks_integer(t.substr(8, 2)) && looks_integer(t.substr(11, 2)) &&
          looks_integer(t.substr(14, 2)) && looks_integer(t.substr(17, 2)));
}

// ----------------------------------------------------------------------
/*!
 * \brief Test if string looks like a epoch time
 */
// ----------------------------------------------------------------------

bool looks_epoch(const std::string& t) { return looks_integer(t); }

// ----------------------------------------------------------------------
/*!
 * \brief Test if string looks like a time offset
 */
// ----------------------------------------------------------------------

bool looks_offset(const std::string& str)
{
  if (str.empty()) return false;

  if (str == "0" || (str.size() == 2 && str[0] == '0') ||  // 0m, 0h etc
      str[0] == '+' || str[0] == '-')
    return true;
  else
    return false;
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse an ISO8601 time duration
 *
 * - https://en.wikipedia.org/wiki/ISO_8601#Durations
 * - https://stackoverflow.com/questions/23886140/parse-iso-8601-durations
 */
//----------------------------------------------------------------------

boost::posix_time::time_duration try_parse_iso_duration(const std::string& str)
{
  boost::smatch match;

  if (boost::regex_search(str, match, iso8601_weeks))
  {
    int n = std::stoi(match[1]);
    return boost::posix_time::hours(7 * 24 * n);
  }

  if (!boost::regex_search(str, match, iso8601_long)) return bad_duration;

  // years, months, days, tmp , hours, minutes, seconds
  std::vector<int> vec{0, 0, 0, -1, 0, 0, 0};

  for (size_t i = 1; i < match.size(); ++i)
  {
    if (match[i].matched && i != 4)
    {
      std::string str = match[i];
      str.pop_back();
      vec[i - 1] = std::stoi(str);
    }
  }

  if (vec[1] < 0 || vec[1] > 12) return bad_duration;
  if (vec[4] < 0 || vec[4] > 24) return bad_duration;

  // Year length 365 and month length 30 are arbitrary choices here

  return boost::posix_time::hours(365 * 24 * vec[0] + 30 * 24 * vec[1] + 24 * vec[2]) +
         boost::posix_time::time_duration(vec[4], vec[5], vec[6], 0);
}

// ----------------------------------------------------------------------
/*!
 * \brief Local date time creator which handles DST changes nicely
 */
// ----------------------------------------------------------------------

boost::local_time::local_date_time make_time(const boost::gregorian::date& date,
                                             const boost::posix_time::time_duration& duration,
                                             const boost::local_time::time_zone_ptr& zone)
{
  namespace bl = boost::local_time;
  namespace bp = boost::posix_time;

  // Handle the normal case

  bl::local_date_time dt(date, duration, zone, bl::local_date_time::NOT_DATE_TIME_ON_ERROR);

  // If the constructed time is not valid, see if we can fix
  // it using DST rules.

  if (dt.is_not_a_date_time())
  {
    // When summer time ends some times will occur twice, and
    // Boost refuses to choose one for you. We have to make the
    // pick, and we choose summer time.

    try
    {
      const bool summertime = true;
      dt = bl::local_date_time(date, duration, zone, summertime);
    }
    catch (...)
    {
      bp::ptime t(date, duration);
      bp::ptime dst_start = zone->dst_local_start_time(date.year());
      if (date == dst_start.date())
      {
        bp::ptime dst_end = dst_start + zone->dst_offset();
        if (t >= dst_start && t <= dst_end)
        {
          dt = bl::local_date_time(date,
                                   duration + zone->dst_offset(),
                                   zone,
                                   bl::local_date_time::NOT_DATE_TIME_ON_ERROR);
        }
        // We'll just return an invalid date if above fails
      }
    }
  }
  return dt;
}

// ----------------------------------------------------------------------
/*!
 * \brief Try to parse iso format
 *
 * \param str The string to parse
 * \param isutc Flag to indicate if the time is in UTC
 * \return The parsed time or an invalid time
 *
 * We ignore
 *
 *  - fractional times
 *  - week format
 *  - day of year format
 */
// ----------------------------------------------------------------------

boost::posix_time::ptime try_parse_iso(const std::string& str, bool* isutc)
{
  unsigned short year = 0;
  unsigned short month = 1, day = 1;
  unsigned short int hour = 0, minute = 0, second = 0;
  unsigned short houroffset = 0, minuteoffset = 0;
  bool positiveoffset = false;

  const char* ptr = str.c_str();

  // By default the time is in local time
  *isutc = false;

  // Year

  if (!parse_ushort(&ptr, 4, &year)) return bad_time;

  // Quick sanity check to prevent further useless parsing
  // - boost library version 1.34 or greater support dates
  //   at least in the range 1400-Jan-01 to 9999-Dec-31
  // - Dates prior to 1582 using the Julian Calendar
  if (year < 1582 || year > 5000) return bad_time;

  // Establish whether we have basic or extended format

  bool extended_format = (*ptr == '-');

  // Month

  if (!skip_separator(&ptr, '-', extended_format)) return bad_time;  // should never happen though
  if (!parse_ushort(&ptr, 2, &month)) return bad_time;               // YYYY is not allowed
  if (month == 0 || month > 12) return bad_time;

  if (*ptr == '\0')
  {
    if (!extended_format) return bad_time;  // YYYYMM is not allowed
    goto build_iso;                         // YYYY-MM is allowed
  }

  // Day

  if (!skip_separator(&ptr, '-', extended_format)) return bad_time;
  if (!parse_ushort(&ptr, 2, &day)) return bad_time;
  if (day == 0 || day > 31) return bad_time;
  if (*ptr == '\0') goto build_iso;  // YYYY-MM-DD is allowed

  // We permit omitting 'T' to enable old YYYYMMDDHHMI timestamp format

  if (*ptr == 'T') ++ptr;
  if (!parse_ushort(&ptr, 2, &hour)) return bad_time;
  if (hour > 23) return bad_time;
  if (*ptr == '\0') goto build_iso;  // YYYY-MM-DDTHH is allowed

  if (*ptr == 'Z' || *ptr == '+' || *ptr == '-') goto zone_began;

  if (!skip_separator(&ptr, ':', extended_format)) return bad_time;
  if (!parse_ushort(&ptr, 2, &minute)) return bad_time;
  if (minute > 59) return bad_time;
  if (*ptr == '\0') goto build_iso;  // YYYY-MM-DDTHH:MI is allowed

  if (*ptr == 'Z' || *ptr == '+' || *ptr == '-') goto zone_began;

  if (!skip_separator(&ptr, ':', extended_format)) return bad_time;
  if (!parse_ushort(&ptr, 2, &second)) return bad_time;
  if (second > 59) return bad_time;
  if (*ptr == '\0') goto build_iso;  // YYYY-MM-DDTHH:MI:SS is allowed

  if (*ptr != 'Z' && *ptr != '+' && *ptr != '-') return bad_time;

zone_began:

  *isutc = true;
  if (*ptr == 'Z')
  {
    ++ptr;
    if (*ptr != '\0') return bad_time;
    goto build_iso;
  }

  positiveoffset = (*ptr == '+');
  ptr++;

  if (!parse_ushort(&ptr, 2, &houroffset)) return bad_time;
  if (houroffset >= 14) return bad_time;  // some offsets are > 12

  if (*ptr == '\0') goto build_iso;

  if (!skip_separator(&ptr, ':', extended_format)) return bad_time;
  if (!parse_ushort(&ptr, 2, &minuteoffset)) return bad_time;
  if (*ptr != '\0') return bad_time;

build_iso:

  boost::posix_time::ptime t(boost::gregorian::date(year, month, day),
                             boost::posix_time::hours(hour) + boost::posix_time::minutes(minute) +
                                 boost::posix_time::seconds(second));

  // Adjust if necessary

  if (houroffset != 0 || minuteoffset != 0)
  {
    if (positiveoffset)
      t -= (boost::posix_time::hours(houroffset) + boost::posix_time::minutes(minuteoffset));
    else
      t += (boost::posix_time::hours(houroffset) + boost::posix_time::minutes(minuteoffset));
  }

  return t;
}

// ----------------------------------------------------------------------
/*!
 * \brief Test if string looks like a iso timestamp
 */
// ----------------------------------------------------------------------

bool looks_iso(const std::string& str)
{
  bool utc;
  boost::posix_time::ptime t = try_parse_iso(str, &utc);
  return !t.is_not_a_date_time();
}

}  // namespace

namespace Fmi
{
// ----------------------------------------------------------------------
/*!
 * \brief DateTime parser implementation details
 */
// ----------------------------------------------------------------------

struct DateTimeParser::Impl
{
  using const_iterator = std::string::const_iterator;
  TimeParser::ISOParser<const_iterator> itsISOParser{};
  TimeParser::FMIParser<const_iterator> itsFMIParser{};
  TimeParser::SQLParser<const_iterator> itsSQLParser{};
  TimeParser::OffsetParser<const_iterator> itsOffsetParser{};
  TimeParser::EpochParser itsEpochParser{};

  boost::posix_time::ptime parse(const std::string& str) const;
  boost::posix_time::ptime parse_iso(const std::string& str) const;
  boost::posix_time::ptime parse_epoch(const std::string& str) const;
  boost::posix_time::ptime parse_sql(const std::string& str) const;
  boost::posix_time::ptime parse_fmi(const std::string& str) const;
  boost::posix_time::ptime parse_offset(const std::string& str) const;

  std::string looks(const std::string& str) const;
  bool looks_utc(const std::string& str) const;

  boost::posix_time::ptime try_parse_offset(const std::string& str) const;

  boost::posix_time::time_duration try_parse_duration(const std::string& str) const;

  boost::posix_time::ptime match_and_parse(const std::string& str, ParserId& matchedParser) const;
};

// ----------------------------------------------------------------------
/*!
 * \brief Parse ISO time format
 */
// ----------------------------------------------------------------------

boost::posix_time::ptime DateTimeParser::Impl::parse_iso(const std::string& str) const
{
  typedef std::string::const_iterator iterator;
  TimeParser::TimeStamp target;

  iterator start = str.begin();
  iterator finish = str.end();

  bool success = qi::parse(start, finish, itsISOParser, target);

  if (success)  // parse succesful, parsers check that entire input was consumed
    return buildFromISO(target);
  throw std::runtime_error("Invalid ISO-time: '" + str + "'");
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse FMI time format
 */
// ----------------------------------------------------------------------

boost::posix_time::ptime DateTimeParser::Impl::parse_fmi(const std::string& str) const
{
  typedef std::string::const_iterator iterator;
  TimeStamp target;

  iterator start = str.begin();
  iterator finish = str.end();

  bool success = qi::parse(start, finish, itsFMIParser, target);

  if (success)  // parse succesful, parsers check that entire input was consumed
    return buildFromISO(target);
  throw std::runtime_error("Invalid ISO-time: '" + str + "'");
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse sql format
 */
// ----------------------------------------------------------------------

boost::posix_time::ptime DateTimeParser::Impl::parse_sql(const std::string& str) const
{
  typedef std::string::const_iterator iterator;
  TimeStamp target;

  iterator start = str.begin();
  iterator finish = str.end();

  bool success = qi::parse(start, finish, itsSQLParser, target);

  if (success)  // parse succesful, parsers check that entire input was consumed
    return buildFromSQL(target);
  throw std::runtime_error("Invalid SQL-time: '" + str + "'");
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse epoch time
 */
// ----------------------------------------------------------------------

boost::posix_time::ptime DateTimeParser::Impl::parse_epoch(const std::string& str) const
{
  typedef std::string::const_iterator iterator;

  UnixTime target;

  iterator start = str.begin();
  iterator finish = str.end();

  bool success = qi::parse(start, finish, itsEpochParser, target);

  if (success)  // parse succesful, parsers check that entire input was consumed
    return buildFromEpoch(target);
  else
    throw std::runtime_error("Invalid epoch time: '" + str + "'");
}

// ----------------------------------------------------------------------
/*!
 * \brief Try to parse a time offset
 *
 * \param str The string to parse
 * \return The parsed time or an invalid time
 */
// ----------------------------------------------------------------------

boost::posix_time::ptime DateTimeParser::Impl::try_parse_offset(const std::string& str) const
{
  if (str.empty()) return bad_time;

  auto offset = try_parse_duration(str);
  if (offset.is_not_a_date_time()) return bad_time;

  return buildFromOffset(offset);
}

// ----------------------------------------------------------------------
/*!
 * \brief Guess the input format
 */
// ----------------------------------------------------------------------

std::string DateTimeParser::Impl::looks(const std::string& str) const
{
  if (looks_offset(str))
    return "offset";
  else if (looks_iso(str))
    return "iso";
  else if (looks_sql(str))
    return "sql";
  else if (looks_epoch(str))
    return "epoch";
  else
    throw std::runtime_error("Unrecognizable time format in string '" + str + "'");
}

// ----------------------------------------------------------------------
/*!
 * \brief Does the time format look like it is in UTC
 */
// ----------------------------------------------------------------------

bool DateTimeParser::Impl::looks_utc(const std::string& str) const
{
  if (looks_sql(str)) return false;
  if (looks_offset(str))  // offsets are always relative to the time now
    return true;

  bool utc;
  boost::posix_time::ptime t = try_parse_iso(str, &utc);
  if (!t.is_not_a_date_time()) return utc;

  if (looks_epoch(str)) return true;

  // Should not be reached now, but is the default mode for
  // any new time format to be added

  return false;
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse a time offset
 */
// ----------------------------------------------------------------------

boost::posix_time::ptime DateTimeParser::Impl::parse_offset(const std::string& str) const
{
  if (str.empty()) throw std::runtime_error("Trying to parse an empty string as a time offset");

  auto offset = try_parse_duration(str);
  if (offset.is_not_a_date_time())
    throw std::runtime_error("Failed to parse '" + str + "' as a duration");

  return buildFromOffset(offset);
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse a time duration
 *
 * Allowed formats include ISO8601 and for historical reasons simple offsets
 *
 *     ISO8601  (P<n>...)
 *     0		(zero offset)
 *     0m,0h... (zero offset with units)
 *     -+NNNN	(offset in minutes)
 *     +-NNNNm	(offset in minutes)
 *     +-NNNNh	(offset in hours)
 *     +-NNNNd	(offset in days)
 *     +-NNNNw	(offset in weeks)
 *     +-NNNNy	(offset in years)
 */
//----------------------------------------------------------------------

boost::posix_time::time_duration DateTimeParser::Impl::try_parse_duration(
    const std::string& str) const
{
  if (str.empty()) return bad_duration;

  if (str[0] == 'P') return try_parse_iso_duration(str);

  // Old time duration format

  typedef std::string::const_iterator iterator;

  TimeOffset target;

  iterator start = str.begin();
  iterator finish = str.end();

  bool success = qi::parse(start, finish, itsOffsetParser, target);

  if (!success) return bad_duration;

  int offset_value;

  // Handle the sign
  if (target.sign == '-')
    offset_value = static_cast<int>(-target.value);
  else
    offset_value = static_cast<int>(target.value);

  // Default unit is minutes
  if (!target.unit) return boost::posix_time::minutes(offset_value);

  char theUnit = *target.unit;

  if (theUnit == 's' || theUnit == 'S') return boost::posix_time::seconds(offset_value);

  if (theUnit == 'm' || theUnit == 'M') return boost::posix_time::minutes(offset_value);

  if (theUnit == 'h' || theUnit == 'H') return boost::posix_time::hours(offset_value);

  if (theUnit == 'd' || theUnit == 'D') return boost::posix_time::hours(offset_value * 24);

  if (theUnit == 'w' || theUnit == 'W') return boost::posix_time::hours(offset_value * 24 * 7);

  if (theUnit == 'y' || theUnit == 'Y') return boost::posix_time::hours(offset_value * 24 * 365);

  return bad_duration;
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse a time string and return the matched parser
 */
// ----------------------------------------------------------------------
boost::posix_time::ptime DateTimeParser::Impl::match_and_parse(const std::string& str,
                                                               ParserId& matchedParser) const
{
  using iterator = std::string::const_iterator;

  {
    TimeStamp target;

    iterator start = str.begin();
    iterator finish = str.end();
    bool success = qi::parse(start, finish, itsISOParser, target);
    if (success)  // parse succesful, parsers check that entire input was consumed
    {
      try
      {
        boost::posix_time::ptime ret;
        ret = buildFromISO(target);
        matchedParser = ISO;
        return ret;
      }
      catch (const std::runtime_error&)
      {
        // Simply pass to the next parser
      }
    }
  }

  {
    TimeStamp target;

    iterator start = str.begin();
    iterator finish = str.end();
    bool success = qi::parse(start, finish, itsFMIParser, target);
    if (success)  // parse succesful, parsers check that entire input was consumed
    {
      try
      {
        boost::posix_time::ptime ret;
        ret = buildFromISO(target);  // Similar building as with ISO parser
        matchedParser = FMI;
        return ret;
      }
      catch (const std::runtime_error&)
      {
        // Simply pass to the next parser
      }
    }
  }

  {
    TimeStamp target;

    iterator start = str.begin();
    iterator finish = str.end();
    bool success = qi::parse(start, finish, itsSQLParser, target);
    if (success)  // parse succesful, parsers check that entire input was consumed
    {
      try
      {
        boost::posix_time::ptime ret;
        ret = buildFromSQL(target);
        matchedParser = SQL;
        return ret;
      }
      catch (const std::runtime_error&)
      {
        // Simply pass to the next parser
      }
    }
  }

  {
    auto ret = try_parse_offset(str);
    if (!ret.is_not_a_date_time())
    {
      matchedParser = OFFSET;
      return ret;
    }
  }

  {
    UnixTime target;

    iterator start = str.begin();
    iterator finish = str.end();
    bool success = qi::parse(start, finish, itsEpochParser, target);
    if (success)  // parse succesful, parsers check that entire input was consumed
    {
      try
      {
        boost::posix_time::ptime ret;
        ret = buildFromEpoch(target);
        matchedParser = EPOCH;
        return ret;
      }
      catch (const std::runtime_error&)
      {
        // Simply pass to the next parser
      }
    }
  }

  // Control is here, no match
  throw std::runtime_error("Unknown time string '" + str + "'");
}

// ----------------------------------------------------------------------
/*!
 * \brief Guess time format and parse the time
 */
// ----------------------------------------------------------------------

boost::posix_time::ptime DateTimeParser::Impl::parse(const std::string& str) const
{
  ParserId unused;
  return match_and_parse(str, unused);
}

// ----------------------------------------------------------------------
/*!
 * \brief Destructor definition for implementation hiding
 */
// ----------------------------------------------------------------------

DateTimeParser::~DateTimeParser() = default;

// ----------------------------------------------------------------------
/*!
 * \brief Constructor initializes Boost Spirit parsers once for reuse speed
 */
// ----------------------------------------------------------------------

DateTimeParser::DateTimeParser() : impl(new DateTimeParser::Impl) {}

// ----------------------------------------------------------------------
/*!
 * \brief Parse a date time in given time zone in the given format
 */
// ----------------------------------------------------------------------

boost::local_time::local_date_time DateTimeParser::parse(const std::string& str,
                                                         const std::string& format,
                                                         boost::local_time::time_zone_ptr tz) const
{
  boost::posix_time::ptime t = parse(str, format);

  // epoch is always in UTC
  if (format == "epoch") return boost::local_time::local_date_time(t, tz);

  // timestamps are local
  return make_time(t.date(), t.time_of_day(), tz);
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse a local date time in the given time zone
 */
// ----------------------------------------------------------------------

boost::local_time::local_date_time DateTimeParser::parse(const std::string& str,
                                                         boost::local_time::time_zone_ptr tz) const
{
  ParserId matched;

  boost::posix_time::ptime t = impl->match_and_parse(str, matched);

  // epoch is always in UTC
  if (matched == EPOCH) return boost::local_time::local_date_time(t, tz);

  // timestamps are local
  return make_time(t.date(), t.time_of_day(), tz);
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse a time in the given format
 */
// ----------------------------------------------------------------------

boost::posix_time::ptime DateTimeParser::parse(const std::string& str,
                                               const std::string& format) const
{
  if (format == "iso" || format == "xml" || format == "timestamp") return impl->parse_iso(str);
  if (format == "sql") return impl->parse_sql(str);
  if (format == "epoch") return impl->parse_epoch(str);
  if (format == "offset") return impl->parse_offset(str);
  if (format == "fmi") return impl->parse_fmi(str);
  throw std::runtime_error("Unknown time format '" + format + "'");
}

// ----------------------------------------------------------------------
/*!
 * \brief Guess time format and parse the time
 */
// ----------------------------------------------------------------------ö

boost::posix_time::ptime DateTimeParser::parse(const std::string& str) const
{
  ParserId unused;
  return impl->match_and_parse(str, unused);
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse a http date
 *
 * From the standard:
 *
 * HTTP applications have historically allowed three different
 * formats for the representation of date/time stamps:
 *
 *   Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
 *   Sunday, 06-Nov-94 08:49:37 GMT ; RFC 850, obsoleted by RFC 1036
 *   Sun Nov  6 08:49:37 1994       ; ANSI C's asctime() format
 *
 * The first format is preferred as an Internet standard and represents
 * a fixed-length subset of that defined by RFC 1123 [8] (an update to
 * RFC 822 [9]). The second format is in common use, but is based on the
 * obsolete RFC 850 [12] date format and lacks a four-digit year.
 * HTTP/1.1 clients and servers that parse the date value MUST accept
 * all three formats (for compatibility with HTTP/1.0), though they MUST
 * only generate the RFC 1123 format for representing HTTP-date values
 * in header fields. See section 19.3 for further information.
 *
 * Note that we do not implement full RFC parsers, since in HTTP
 * the dates are always in GMT time.
 */
// ----------------------------------------------------------------------

boost::posix_time::ptime DateTimeParser::parse_http(const std::string& str) const
{
  if (str.empty()) throw std::runtime_error("Empty string is not a HTTP date");

  std::string s = boost::algorithm::replace_all_copy(str, "  ", " ");

  try
  {
    std::vector<std::string> parts;
    boost::algorithm::split(parts, s, boost::algorithm::is_any_of(" "));

    unsigned short dd, yy, mm;
    unsigned short hh, mi, ss;
    std::string hms;

    switch (parts.size())
    {
      case 6:  // RFC822: Sun, 06 Nov 1994 08:49:37 GMT
      {
        if (!is_short_weekday(parts[0].substr(0, 3)) || parts[0].substr(3, 1) != "," ||
            !is_short_month(parts[2]) || parts[5] != "GMT")
        {
          throw std::runtime_error("");
        }
        dd = std::stoul(parts[1]);
        yy = std::stoul(parts[3]);
        mm = get_short_month(parts[2]);
        hms = parts[4];
        break;
      }
      case 4:  // RFC 850: Sunday, 06-Nov-94 08:49:37 GMT
      {
        if (!is_long_weekday(parts[0].substr(0, parts[0].size() - 1)) ||
            parts[0].substr(parts[0].size() - 1, 1) != "," ||
            !is_short_month(parts[1].substr(3, 3)) || parts[3] != "GMT")
        {
          throw std::runtime_error("");
        }
        dd = std::stoul(parts[1].substr(0, 2));
        yy = std::stoul(parts[1].substr(7, 2));
        yy += (yy < 50 ? 2000 : 1900);
        mm = get_short_month(parts[1].substr(3, 3));
        hms = parts[2];
        break;
      }
      case 5:  // asctime: Sun Nov  6 08:49:37 1994
      {
        if (!is_short_weekday(parts[0]) || !is_short_month(parts[1]))
        {
          throw std::runtime_error("");
        }
        dd = std::stoul(parts[2]);
        yy = std::stoul(parts[4]);
        mm = get_short_month(parts[1]);
        hms = parts[3];
        break;
      }
      default:
        throw std::runtime_error("Invalid HTTP date: " + str);
    }

    hh = std::stoul(hms.substr(0, 2));
    mi = std::stoul(hms.substr(3, 2));
    ss = std::stoul(hms.substr(6, 2));

    boost::posix_time::ptime t(boost::gregorian::date(yy, mm, dd),
                               boost::posix_time::hours(hh) + boost::posix_time::minutes(mi) +
                                   boost::posix_time::seconds(ss));

    if (t.is_not_a_date_time()) throw std::runtime_error("");

    return t;
  }
  catch (...)
  {
    throw std::runtime_error("Not a HTTP-date: " + str);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse a time duration
 *
 * Allowed formats include ISO8601 and for historical reasons simple offsets
 *
 *     ISO8601  (P<n>...)
 *     0		(zero offset)
 *     0m,0h... (zero offset with units)
 *     -+NNNN	(offset in minutes)
 *     +-NNNNm	(offset in minutes)
 *     +-NNNNh	(offset in hours)
 *     +-NNNNd	(offset in days)
 *     +-NNNNw	(offset in weeks)
 *     +-NNNNy	(offset in years)
 */
//----------------------------------------------------------------------

boost::posix_time::time_duration DateTimeParser::parse_duration(const std::string& str) const
{
  if (str.empty()) throw std::runtime_error("Trying to parse an empty string as a time duration");
  auto dura = impl->try_parse_duration(str);
  if (dura.is_not_a_date_time())
    throw std::runtime_error("Failed to parse '" + str + "' as a duration");
  return dura;
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse an ISO8601 time duration
 *
 * - https://en.wikipedia.org/wiki/ISO_8601#Durations
 * - https://stackoverflow.com/questions/23886140/parse-iso-8601-durations
 */
//----------------------------------------------------------------------

boost::posix_time::time_duration DateTimeParser::parse_iso_duration(const std::string& str) const
{
  auto dura = try_parse_iso_duration(str);

  if (!dura.is_not_a_date_time()) return dura;

  throw std::runtime_error("Unable to parse ISO8601 time duration from '" + str + "'");
}

}  // namespace Fmi

// ======================================================================