#include "StringConversion.h"

#include <fmt/format.h>
#if defined(_WIN32) || defined(WIN32)
#include <fmt/printf.h>
#endif

#include <boost/numeric/conversion/cast.hpp>  // numeric_cast
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/spirit/include/qi.hpp>

#include <cmath>
#include <stdexcept>

namespace Fmi
{
// ----------------------------------------------------------------------
/*
 * Convert numbers to strings without using the global locale.
 * The format specifiers are the same as in the standard, except
 * that we use %g instead of %g to avoid unnecessary trailing zeros.
 */
// ----------------------------------------------------------------------

std::string to_string(int value) { return fmt::sprintf("%d", value); }
std::string to_string(long value) { return fmt::sprintf("%ld", value); }
std::string to_string(unsigned int value) { return fmt::sprintf("%u", value); }
std::string to_string(unsigned long value) { return fmt::sprintf("%lu", value); }
#if defined(_WIN32) || defined(WIN32)
std::string to_string(size_t value) { return fmt::sprintf("%zu", value); }
std::string to_string(time_t value) { return fmt::sprintf("%zd", value); }
#endif
std::string to_string(float value) { return fmt::sprintf("%g", value); }
std::string to_string(double value) { return fmt::sprintf("%g", value); }
std::string to_string(const char* fmt, int value) { return fmt::sprintf(fmt, value); }
std::string to_string(const char* fmt, long value) { return fmt::sprintf(fmt, value); }
std::string to_string(const char* fmt, unsigned int value) { return fmt::sprintf(fmt, value); }
std::string to_string(const char* fmt, unsigned long value) { return fmt::sprintf(fmt, value); }
std::string to_string(const char* fmt, float value) { return fmt::sprintf(fmt, value); }
std::string to_string(const char* fmt, double value) { return fmt::sprintf(fmt, value); }
// ----------------------------------------------------------------------
/*
 * Convert strings to numbers.
 *
 * Throws:
 *
 *  std::invalid_argument if no conversion could be performed
 *  std::bad_cast if the converted value would fall out of
 *  the range of the result type or if the underlying function.
 */
// ----------------------------------------------------------------------

int stoi(const std::string& str)
{
  long result;
  std::string::const_iterator begin = str.begin(), end = str.end();
  if (boost::spirit::qi::parse(begin, end, boost::spirit::qi::long_, result))
    if (begin == end) return boost::numeric_cast<int>(result);
  throw std::invalid_argument("Fmi::stoi failed to convert '" + str + "' to integer");
}

long stol(const std::string& str)
{
  long result;
  std::string::const_iterator begin = str.begin(), end = str.end();
  if (boost::spirit::qi::parse(begin, end, boost::spirit::qi::long_, result))
    if (begin == end) return result;
  throw std::invalid_argument("Fmi::stol failed to convert '" + str + "' to long");
}

unsigned long stoul(const std::string& str)
{
  unsigned long result;
  std::string::const_iterator begin = str.begin(), end = str.end();
  if (boost::spirit::qi::parse(begin, end, boost::spirit::qi::ulong_, result))
    if (begin == end) return result;
  throw std::invalid_argument("Fmi::stoul failed to convert '" + str + "' to unsigned long");
}

float stof(const std::string& str)
{
  // We parse as double because of this:
  // http://stackoverflow.com/questions/17391348/boost-spirit-floating-number-parser-precision

  double result;
  std::string::const_iterator begin = str.begin(), end = str.end();
  if (boost::spirit::qi::parse(begin, end, boost::spirit::qi::double_, result))
    if (begin == end)
    {
      if (std::isfinite(result)) return boost::numeric_cast<float>(result);
      throw std::invalid_argument("Infinite numbers are not allowed: '" + str + "' in Fmi::stof");
    }
  throw std::invalid_argument("Fmi::stof failed to convert '" + str + "' to float");
}

double stod(const std::string& str)
{
  double result;
  std::string::const_iterator begin = str.begin(), end = str.end();
  if (boost::spirit::qi::parse(begin, end, boost::spirit::qi::double_, result))
    if (begin == end)
    {
      if (std::isfinite(result)) return result;
      throw std::invalid_argument("Infinite numbers are not allowed: '" + str + "' in Fmi::stod");
    }
  throw std::invalid_argument("Fmi::stod failed to convert '" + str + "' to double");
}

// ----------------------------------------------------------------------
/*
 * Convert dates and times to strings. The code mimics the respective
 * code in boost, but avoids using the locale by using fmt::sprintf.
 */
// ----------------------------------------------------------------------

// Convert a duration to iso string of form HHMMSS[,fffffff]
std::string to_iso_string(const boost::posix_time::time_duration& duration)
{
  if (duration.is_special())
  {
    if (duration.is_not_a_date_time())
      return "not-a-date-time";
    else if (duration.is_pos_infinity())
      return "+infinity";
    else if (duration.is_neg_infinity())
      return "-infinity";
    else
      return "";  // this case does not exist in boost 1.59
  }
  else
  {
    std::string ret;
    ret.reserve(1 + 6 + 1);  // sign + hhmmss + terminator byte, hoping for no fractional part
    if (duration.is_negative()) ret += '-';
    ret +=
        fmt::sprintf("%02ld%02ld%02ld", duration.hours(), duration.minutes(), duration.seconds());
    auto frac_sec = duration.fractional_seconds();
    if (frac_sec != 0)
    {
      std::string fmt = ",%0" + Fmi::to_string(duration.num_fractional_digits()) + "ld";
      ret += fmt::sprintf(fmt, frac_sec);
    }
    return ret;
  }
}

// Convert a duration to string of form HH:MM:SS[,fffffff]
std::string to_iso_extended_string(const boost::posix_time::time_duration& duration)
{
  if (duration.is_special())
  {
    if (duration.is_not_a_date_time())
      return "not-a-date-time";
    else if (duration.is_pos_infinity())
      return "+infinity";
    else if (duration.is_neg_infinity())
      return "-infinity";
    else
      return "";  // this case does not exist in boost 1.59
  }
  else
  {
    std::string ret;
    ret.reserve(1 + 8 + 1);  // sign + hh:mm:ss + terminator byte, hoping for no fractional part
    if (duration.is_negative()) ret += '-';
    ret +=
        fmt::sprintf("%02ld:%02ld:%02ld", duration.hours(), duration.minutes(), duration.seconds());
    auto frac_sec = duration.fractional_seconds();
    if (frac_sec != 0)
    {
      std::string fmt = "%0" + Fmi::to_string(duration.num_fractional_digits()) + "ld";
      ret += fmt::sprintf("," + fmt, frac_sec);
    }
    return ret;
  }
}

// Convert date to form YYYYMMDD
std::string to_iso_string(const boost::gregorian::date& date)
{
  boost::gregorian::greg_year_month_day ymd = date.year_month_day();
  return fmt::sprintf("%04d%02d%02d",
                      static_cast<int>(ymd.year),
                      static_cast<int>(ymd.month),
                      static_cast<int>(ymd.day));
}

// Convert date to form YYYY-MM-DD
std::string to_iso_extended_string(const boost::gregorian::date& date)
{
  boost::gregorian::greg_year_month_day ymd = date.year_month_day();
  return fmt::sprintf("%04d-%02d-%02d",
                      static_cast<int>(ymd.year),
                      static_cast<int>(ymd.month),
                      static_cast<int>(ymd.day));
}

// Convert to form YYYYMMDDTHHMMSS,fffffffff where T is the date-time separator
std::string to_iso_string(const boost::posix_time::ptime& time)
{
  const auto& date = time.date();
  const auto& duration = time.time_of_day();
  std::string ret;
  ret.reserve(15 + 1);  // +1 for null byte terminator, we hope there is no fractional part
  ret += to_iso_string(date);
  if (!duration.is_special()) ret.append("T").append(to_iso_string(duration));
  return ret;
}

// Convert to form YYYY-MM-DDTHH:MM:SS,fffffffff where T is the date-time separator
std::string to_iso_extended_string(const boost::posix_time::ptime& time)
{
  const auto& date = time.date();
  const auto& duration = time.time_of_day();
  std::string ret;
  ret.reserve(19 + 1);  // +1 for null byte terminator, we hope there is no fractional part
  ret += to_iso_extended_string(date);
  if (!duration.is_special()) ret.append("T").append(to_iso_extended_string(duration));
  return ret;
}

// Convert to lower case with ASCII input
void ascii_tolower(std::string& input)
{
  for (std::string::iterator it = input.begin(); it != input.end(); ++it)
  {
    if (*it > 64 && *it < 91) *it += 32;
  }
}

// Convert to upper case with ASCII input
void ascii_toupper(std::string& input)
{
  for (std::string::iterator it = input.begin(); it != input.end(); ++it)
  {
    if (*it > 96 && *it < 123) *it -= 32;
  }
}

// ASCII lower case with copy
std::string ascii_tolower_copy(std::string input)
{
  ascii_tolower(input);
  return input;
}

// ASCII upper case with copy
std::string ascii_toupper_copy(std::string input)
{
  ascii_toupper(input);
  return input;
}

}  // namespace Fmi
