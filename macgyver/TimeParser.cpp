// ======================================================================
/*!
 * \brief Parse timestamps
 */
// ======================================================================

#include "TimeParser.h"
#include "TimeParserDefinitions.h"
#include "Cast.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

#include <stdexcept>
#include <cctype>

namespace
{
boost::posix_time::ptime buildFromSQL(const Fmi::TimeParser::TimeStamp& target)
{
  unsigned int hour = 0, minute = 0, second = 0;
  if (target.hour)
  {
    hour = *target.hour;
  }
  if (target.minute)
  {
    minute = *target.minute;
  }
  if (target.second)
  {
    second = *target.second;
  }

  boost::posix_time::ptime ret;

  // Translate the exception to runtime_error
  try
  {
    ret = boost::posix_time::ptime(boost::gregorian::date(target.year, target.month, target.day),
                                   boost::posix_time::hours(hour) +
                                       boost::posix_time::minutes(minute) +
                                       boost::posix_time::seconds(second));
  }
  catch (std::exception& err)
  {
    throw std::runtime_error(err.what());
  }

  return ret;
}

boost::posix_time::ptime buildFromISO(const Fmi::TimeParser::TimeStamp& target)
{
  unsigned int hour = 0, minute = 0, second = 0;

  if (target.hour)
  {
    hour = *target.hour;
  }
  if (target.minute)
  {
    minute = *target.minute;
  }
  if (target.second)
  {
    second = *target.second;
  }

  boost::posix_time::ptime res;

  try
  {
    res = boost::posix_time::ptime(boost::gregorian::date(target.year, target.month, target.day),
                                   boost::posix_time::hours(hour) +
                                       boost::posix_time::minutes(minute) +
                                       boost::posix_time::seconds(second));
  }
  catch (std::exception& err)
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

boost::posix_time::ptime buildFromEpoch(const Fmi::TimeParser::UnixTime& target)
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
}

namespace Fmi
{
namespace TimeParser
{
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
 * \brief Test if string looks like a iso timestamp
 */
// ----------------------------------------------------------------------

bool looks_iso(const std::string& str)
{
  bool utc;
  boost::posix_time::ptime t = try_parse_iso(str, &utc);
  return !t.is_not_a_date_time();
}

// ----------------------------------------------------------------------
/*!
 * \brief Test if string looks like a time offset
 */
// ----------------------------------------------------------------------

bool looks_offset(const std::string& str)
{
  if (str.empty()) return false;

  if (str == "0" || (str.size() == 2 && str[0] == '0') ||  // 0m, 0h etc
      str[0] == '+' ||
      str[0] == '-')
    return true;
  else
    return false;
}

// ----------------------------------------------------------------------
/*!
 * \brief Guess the input format
 */
// ----------------------------------------------------------------------

std::string looks(const std::string& str)
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

bool looks_utc(const std::string& str)
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
 * \brief Parse epoch time
 */
// ----------------------------------------------------------------------

boost::posix_time::ptime parse_epoch(const std::string& str)
{
  typedef std::string::const_iterator iterator;

  EpochParser theParser;
  UnixTime target;

  iterator start = str.begin();
  iterator finish = str.end();

  bool success = qi::parse(start, finish, theParser, target);

  if (success)  // parse succesful, parsers check that entire input was consumed
  {
    return ::buildFromEpoch(target);
  }
  else
  {
    throw std::runtime_error("Invalid epoch time: '" + str + "'");
  }
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
  static boost::posix_time::ptime badtime;

  unsigned short year = 0;
  unsigned short month = 1, day = 1;
  unsigned short int hour = 0, minute = 0, second = 0;
  unsigned short houroffset = 0, minuteoffset = 0;
  bool positiveoffset = false;

  const char* ptr = str.c_str();

  // By default the time is in local time
  *isutc = false;

  // Year

  if (!parse_ushort(&ptr, 4, &year)) return badtime;

  // Quick sanity check to prevent further useless parsing
  // - boost library version 1.34 or greater support dates
  //   at least in the range 1400-Jan-01 to 9999-Dec-31
  // - Dates prior to 1582 using the Julian Calendar
  if (year < 1582 || year > 5000) return badtime;

  // Establish whether we have basic or extended format

  bool extended_format = (*ptr == '-');

  // Month

  if (!skip_separator(&ptr, '-', extended_format)) return badtime;  // should never happen though
  if (!parse_ushort(&ptr, 2, &month)) return badtime;               // YYYY is not allowed
  if (month == 0 || month > 12) return badtime;

  if (*ptr == '\0')
  {
    if (!extended_format) return badtime;  // YYYYMM is not allowed
    goto build_iso;                        // YYYY-MM is allowed
  }

  // Day

  if (!skip_separator(&ptr, '-', extended_format)) return badtime;
  if (!parse_ushort(&ptr, 2, &day)) return badtime;
  if (day == 0 || day > 31) return badtime;
  if (*ptr == '\0') goto build_iso;  // YYYY-MM-DD is allowed

  // We permit omitting 'T' to enable old YYYYMMDDHHMI timestamp format

  if (*ptr == 'T') ++ptr;
  if (!parse_ushort(&ptr, 2, &hour)) return badtime;
  if (hour > 23) return badtime;
  if (*ptr == '\0') goto build_iso;  // YYYY-MM-DDTHH is allowed

  if (*ptr == 'Z' || *ptr == '+' || *ptr == '-') goto zone_began;

  if (!skip_separator(&ptr, ':', extended_format)) return badtime;
  if (!parse_ushort(&ptr, 2, &minute)) return badtime;
  if (minute > 59) return badtime;
  if (*ptr == '\0') goto build_iso;  // YYYY-MM-DDTHH:MI is allowed

  if (*ptr == 'Z' || *ptr == '+' || *ptr == '-') goto zone_began;

  if (!skip_separator(&ptr, ':', extended_format)) return badtime;
  if (!parse_ushort(&ptr, 2, &second)) return badtime;
  if (second > 59) return badtime;
  if (*ptr == '\0') goto build_iso;  // YYYY-MM-DDTHH:MI:SS is allowed

  if (*ptr != 'Z' && *ptr != '+' && *ptr != '-') return badtime;

zone_began:

  *isutc = true;
  if (*ptr == 'Z')
  {
    ++ptr;
    if (*ptr != '\0') return badtime;
    goto build_iso;
  }

  positiveoffset = (*ptr == '+');
  ptr++;

  if (!parse_ushort(&ptr, 2, &houroffset)) return badtime;
  if (houroffset >= 14) return badtime;  // some offsets are > 12

  if (*ptr == '\0') goto build_iso;

  if (!skip_separator(&ptr, ':', extended_format)) return badtime;
  if (!parse_ushort(&ptr, 2, &minuteoffset)) return badtime;
  if (*ptr != '\0') return badtime;

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
 * \brief Parse ISO time format
 */
// ----------------------------------------------------------------------

boost::posix_time::ptime parse_iso(const std::string& str)
{
  typedef std::string::const_iterator iterator;
  ISOParser<iterator> theParser;
  TimeStamp target;

  iterator start = str.begin();
  iterator finish = str.end();

  bool success = qi::parse(start, finish, theParser, target);

  if (success)  // parse succesful, parsers check that entire input was consumed
  {
    return ::buildFromISO(target);
  }
  else
  {
    throw std::runtime_error("Invalid ISO-time: '" + str + "'");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse FMI time format
 */
// ----------------------------------------------------------------------

boost::posix_time::ptime parse_fmi(const std::string& str)
{
  typedef std::string::const_iterator iterator;
  FMIParser<iterator> theParser;
  TimeStamp target;

  iterator start = str.begin();
  iterator finish = str.end();

  bool success = qi::parse(start, finish, theParser, target);

  if (success)  // parse succesful, parsers check that entire input was consumed
  {
    return ::buildFromISO(target);
  }
  else
  {
    throw std::runtime_error("Invalid ISO-time: '" + str + "'");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse sql format
 */
// ----------------------------------------------------------------------

boost::posix_time::ptime parse_sql(const std::string& str)
{
  typedef std::string::const_iterator iterator;
  SQLParser<iterator> theParser;
  TimeStamp target;

  iterator start = str.begin();
  iterator finish = str.end();

  bool success = qi::parse(start, finish, theParser, target);

  if (success)  // parse succesful, parsers check that entire input was consumed
  {
    return ::buildFromSQL(target);
  }
  else
  {
    throw std::runtime_error("Invalid SQL-time: '" + str + "'");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse a time offset
 */
// ----------------------------------------------------------------------

boost::posix_time::ptime parse_offset(const std::string& str)
{
  if (str.empty()) throw std::runtime_error("Trying to parse an empty string as a time offset");

  boost::posix_time::time_duration offset = parse_duration(str);

  return ::buildFromOffset(offset);
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse a time duration
 *
 * Allowed formats:
 *
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

boost::posix_time::time_duration parse_duration(const std::string& str)
{
  typedef std::string::const_iterator iterator;

  if (str.empty()) throw std::runtime_error("Trying to parse an empty string as a time duration");

  OffsetParser<iterator> theParser;
  TimeOffset target;

  iterator start = str.begin();
  iterator finish = str.end();

  bool success = qi::parse(start, finish, theParser, target);

  if (success)  // parse succesful, parsers check that entire input was consumed
  {
    int offset_value;
    // Handle the sign
    if (target.sign == '-')
    {
      offset_value = static_cast<int>(-target.value);
    }
    else
    {
      offset_value = static_cast<int>(target.value);
    }

    if (target.unit)
    {
      char theUnit = *target.unit;

      if (theUnit == 's' || theUnit == 'S')
      {
        return boost::posix_time::seconds(offset_value);
      }
      else if (theUnit == 'm' || theUnit == 'M')
      {
        return boost::posix_time::minutes(offset_value);
      }
      else if (theUnit == 'h' || theUnit == 'H')
      {
        return boost::posix_time::hours(offset_value);
      }
      else if (theUnit == 'd' || theUnit == 'D')
      {
        return boost::posix_time::hours(offset_value * 24);
      }
      else if (theUnit == 'w' || theUnit == 'W')
      {
        return boost::posix_time::hours(offset_value * 24 * 7);
      }
      else if (theUnit == 'y' || theUnit == 'Y')
      {
        return boost::posix_time::hours(offset_value * 24 * 365);
      }
      else
      {
        throw std::runtime_error(std::string("Unsupported offset specifier: ") + theUnit);
      }
    }
    else
    {
      // No unit, means minutes
      return boost::posix_time::minutes(offset_value);
    }
  }
  else
  {
    throw std::runtime_error("Parse failed for input: " + str);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse a time string and return the matched parser
 */
// ----------------------------------------------------------------------
boost::posix_time::ptime match_and_parse(const std::string& str, ParserId& matchedParser)
{
  typedef std::string::const_iterator iterator;

  {
    FMIParser<iterator> theParser;
    TimeStamp target;

    iterator start = str.begin();
    iterator finish = str.end();
    bool success = qi::parse(start, finish, theParser, target);
    if (success)  // parse succesful, parsers check that entire input was consumed
    {
      try
      {
        boost::posix_time::ptime ret;
        ret = ::buildFromISO(target);  // Similar building as with ISO parser
        matchedParser = FMI;
        return ret;
      }
      catch (std::runtime_error&)
      {
        // Simply pass to the next parser
      }
    }
  }

  {
    ISOParser<iterator> theParser;
    TimeStamp target;

    iterator start = str.begin();
    iterator finish = str.end();
    bool success = qi::parse(start, finish, theParser, target);
    if (success)  // parse succesful, parsers check that entire input was consumed
    {
      try
      {
        boost::posix_time::ptime ret;
        ret = ::buildFromISO(target);
        matchedParser = ISO;
        return ret;
      }
      catch (std::runtime_error&)
      {
        // Simply pass to the next parser
      }
    }
  }

  {
    SQLParser<iterator> theParser;
    TimeStamp target;

    iterator start = str.begin();
    iterator finish = str.end();
    bool success = qi::parse(start, finish, theParser, target);
    if (success)  // parse succesful, parsers check that entire input was consumed
    {
      try
      {
        boost::posix_time::ptime ret;
        ret = ::buildFromSQL(target);
        matchedParser = SQL;
        return ret;
      }
      catch (std::runtime_error&)
      {
        // Simply pass to the next parser
      }
    }
  }

  {
    try
    {
      boost::posix_time::ptime ret;
      ret = parse_offset(str);
      matchedParser = OFFSET;
      return ret;
    }
    catch (std::runtime_error&)
    {
      // Simply pass to the next parser
    }
  }

  {
    EpochParser theParser;
    UnixTime target;

    iterator start = str.begin();
    iterator finish = str.end();
    bool success = qi::parse(start, finish, theParser, target);
    if (success)  // parse succesful, parsers check that entire input was consumed
    {
      try
      {
        boost::posix_time::ptime ret;
        ret = ::buildFromEpoch(target);
        matchedParser = EPOCH;
        return ret;
      }
      catch (std::runtime_error&)
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
 * \brief Parse a time in the given format
 */
// ----------------------------------------------------------------------

boost::posix_time::ptime parse(const std::string& str, const std::string& format)
{
  if (format == "iso" || format == "xml" || format == "timestamp")
    return parse_iso(str);
  else if (format == "sql")
    return parse_sql(str);
  else if (format == "epoch")
    return parse_epoch(str);
  else if (format == "offset")
    return parse_offset(str);
  else if (format == "fmi")
    return parse_fmi(str);
  else
    throw std::runtime_error("Unknown time format '" + format + "'");
}

// ----------------------------------------------------------------------
/*!
 * \brief Guess time format and parse the time
 */
// ----------------------------------------------------------------------ö

boost::posix_time::ptime parse(const std::string& str)
{
  ParserId unused;
  return match_and_parse(str, unused);
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse a time in the given format
 */
// ----------------------------------------------------------------------

boost::local_time::local_date_time parse(const std::string& str,
                                         const std::string& format,
                                         boost::local_time::time_zone_ptr tz)
{
  boost::posix_time::ptime t = parse(str, format);

  // epoch is always in UTC
  if (format == "epoch") return boost::local_time::local_date_time(t, tz);

  // timestamps are local
  return make_time(t.date(), t.time_of_day(), tz);
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse a time
 */
// ----------------------------------------------------------------------

boost::local_time::local_date_time parse(const std::string& str,
                                         boost::local_time::time_zone_ptr tz)
{
  ParserId matched;

  boost::posix_time::ptime t = match_and_parse(str, matched);

  // epoch is always in UTC
  if (matched == EPOCH) return boost::local_time::local_date_time(t, tz);

  // timestamps are local
  return make_time(t.date(), t.time_of_day(), tz);
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

boost::posix_time::ptime parse_http(const std::string& str)
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
        dd = number_cast<unsigned short>(parts[1]);
        yy = number_cast<unsigned short>(parts[3]);
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
        dd = number_cast<unsigned short>(parts[1].substr(0, 2));
        yy = number_cast<unsigned short>(parts[1].substr(7, 2));
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
        dd = number_cast<unsigned short>(parts[2]);
        yy = number_cast<unsigned short>(parts[4]);
        mm = get_short_month(parts[1]);
        hms = parts[3];
        break;
      }
      default:
        throw std::runtime_error("Invalid HTTP date: " + str);
    }

    hh = number_cast<unsigned short>(hms.substr(0, 2));
    mi = number_cast<unsigned short>(hms.substr(3, 2));
    ss = number_cast<unsigned short>(hms.substr(6, 2));

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
}
}  // namespace Fmi::TimeParser

// ======================================================================
