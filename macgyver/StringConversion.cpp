#include "StringConversion.h"
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>  // for is_special() cases
#include <boost/numeric/conversion/cast.hpp>               // numeric_cast
#include <boost/spirit/include/qi.hpp>
#include <fmt/format.h>
#include <fmt/printf.h>
#include <cmath>
#include <stdexcept>

namespace
{
const char* weekdays = "SunMonTueWedThuFriSatSun";
const char* months = "   JanFebMarAprMayJunJulAugSepOctNovDec";

const char digits[] =
    "0001020304050607080910111213141516171819"
    "2021222324252627282930313233343536373839"
    "4041424344454647484950515253545556575859"
    "6061626364656667686970717273747576777879"
    "8081828384858687888990919293949596979899";

}  // namespace

namespace Fmi
{
// ----------------------------------------------------------------------
/*
 * Convert numbers to strings without using the global locale.
 * The format specifiers are the same as in the standard, except
 * that we use %g instead of %g to avoid unnecessary trailing zeros.
 */
// ----------------------------------------------------------------------

std::string to_string(bool value) { return value ? "1" : "0"; }
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
 * code in boost, but avoids using the locale. Code heavily influenced by {fmt}.
 * Note that it might be tempting to make inlined functions for some of the
 * common code segments. However, g++ might not actually inline the code
 * and the result has been observed to be 16 times slower.
 */
// ----------------------------------------------------------------------

// Convert a duration to iso string of form HHMMSS[,fffffff]
std::string to_iso_string(const boost::posix_time::time_duration& duration)
{
  if (duration.is_special()) return boost::posix_time::to_iso_string(duration);

  char buffer[8];
  char* ptr = buffer + 8;
  *--ptr = '\0';
  unsigned index = duration.seconds() * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  index = duration.minutes() * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  index = duration.hours() * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  if (duration.is_negative()) *--ptr = '-';
  std::string ret(ptr);

  auto frac_sec = duration.fractional_seconds();
  if (frac_sec != 0)
  {
    std::string fmt = ",%0" + Fmi::to_string(duration.num_fractional_digits()) + "ld";
    ret += fmt::sprintf(fmt, frac_sec);
  }
  return ret;
}

// Convert a duration to string of form HH:MM:SS[,fffffff]
std::string to_simple_string(const boost::posix_time::time_duration& duration)
{
  if (duration.is_special()) return boost::posix_time::to_simple_string(duration);

  char buffer[10];
  char* ptr = buffer + 10;
  *--ptr = '\0';
  unsigned index = duration.seconds() * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  *--ptr = ':';
  index = duration.minutes() * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  *--ptr = ':';
  index = duration.hours() * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  if (duration.is_negative()) *--ptr = '-';
  std::string ret(ptr);

  auto frac_sec = duration.fractional_seconds();
  if (frac_sec != 0)
  {
    std::string fmt = ",%0" + Fmi::to_string(duration.num_fractional_digits()) + "ld";
    ret += fmt::sprintf(fmt, frac_sec);
  }
  return ret;
}

std::string to_iso_extended_string(const boost::posix_time::time_duration& duration)
{
  return Fmi::to_simple_string(duration);
}

// Convert date to form YYYY-mmm-DD string where mmm 3 char month name
std::string to_simple_string(const boost::gregorian::date& date)
{
  boost::gregorian::greg_year_month_day ymd = date.year_month_day();
  char buffer[12];
  char* ptr = buffer + 12;
  unsigned index = ymd.day * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  *--ptr = '-';
  index = ymd.month * 3;
  *--ptr = months[index + 2];
  *--ptr = months[index + 1];
  *--ptr = months[index];
  *--ptr = '-';
  index = (ymd.year % 100) * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  index = (ymd.year / 100) * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  return std::string(ptr);
}

// Convert date to form YYYYMMDD
std::string to_iso_string(const boost::gregorian::date& date)
{
  boost::gregorian::greg_year_month_day ymd = date.year_month_day();
  char buffer[9];
  char* ptr = buffer + 9;
  *--ptr = '\0';
  unsigned index = ymd.day * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  index = ymd.month * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  index = (ymd.year % 100) * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  index = (ymd.year / 100) * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  return std::string(ptr);
}

// Convert date to form YYYY-MM-DD
std::string to_iso_extended_string(const boost::gregorian::date& date)
{
  boost::gregorian::greg_year_month_day ymd = date.year_month_day();
  char buffer[11];
  char* ptr = buffer + 11;
  *--ptr = '\0';
  unsigned index = ymd.day * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  *--ptr = '-';
  index = ymd.month * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  *--ptr = '-';
  index = (ymd.year % 100) * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  index = (ymd.year / 100) * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  return std::string(ptr);
}

// Convert to form YYYYMMDDTHHMMSS,fffffffff where T is the date-time separator
std::string to_iso_string(const boost::posix_time::ptime& time)
{
  if (time.is_special()) return boost::posix_time::to_iso_string(time);

  const auto& date = time.date();
  const auto& duration = time.time_of_day();
  boost::gregorian::greg_year_month_day ymd = date.year_month_day();

  char buffer[16];
  char* ptr = buffer + 16;
  *--ptr = '\0';
  unsigned index = duration.seconds() * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  index = duration.minutes() * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  index = duration.hours() * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  *--ptr = 'T';
  index = ymd.day * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  index = ymd.month * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  index = (ymd.year % 100) * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  index = (ymd.year / 100) * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  std::string ret(ptr);

  auto frac_sec = duration.fractional_seconds();
  if (frac_sec != 0)
  {
    std::string fmt = ",%0" + Fmi::to_string(duration.num_fractional_digits()) + "ld";
    ret += fmt::sprintf(fmt, frac_sec);
  }
  return ret;
}

// Convert to form YYYYMMDDTHHMM
std::string to_timestamp_string(const boost::posix_time::ptime& time)
{
  if (time.is_special()) return boost::posix_time::to_iso_string(time);

  const auto& date = time.date();
  const auto& duration = time.time_of_day();
  boost::gregorian::greg_year_month_day ymd = date.year_month_day();

  char buffer[13];
  char* ptr = buffer + 13;
  *--ptr = '\0';
  auto index = duration.minutes() * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  index = duration.hours() * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  index = ymd.day * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  index = ymd.month * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  index = (ymd.year % 100) * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  index = (ymd.year / 100) * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  return std::string(ptr);
}

// Convert to form YYYY-MM-DDTHH:MM:SS,fffffffff where T is the date-time separator
std::string to_iso_extended_string(const boost::posix_time::ptime& time)
{
  if (time.is_special()) return boost::posix_time::to_iso_extended_string(time);

  const auto& date = time.date();
  const auto& duration = time.time_of_day();
  boost::gregorian::greg_year_month_day ymd = date.year_month_day();

  char buffer[20];
  char* ptr = buffer + 20;
  *--ptr = '\0';
  unsigned index = duration.seconds() * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  *--ptr = ':';
  index = duration.minutes() * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  *--ptr = ':';
  index = duration.hours() * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  *--ptr = 'T';
  index = ymd.day * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  *--ptr = '-';
  index = ymd.month * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  *--ptr = '-';
  index = (ymd.year % 100) * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  index = (ymd.year / 100) * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  std::string ret(ptr);

  auto frac_sec = duration.fractional_seconds();
  if (frac_sec != 0)
  {
    std::string fmt = ",%0" + Fmi::to_string(duration.num_fractional_digits()) + "ld";
    ret += fmt::sprintf(fmt, frac_sec);
  }
  return ret;
}

// Convert to form YYYY-mmm-DD HH:MM:SS.fffffffff string where mmm 3 char month name
std::string to_simple_string(const boost::posix_time::ptime& time)
{
  if (time.is_special()) return boost::posix_time::to_simple_string(time);

  const auto& date = time.date();
  const auto& duration = time.time_of_day();
  boost::gregorian::greg_year_month_day ymd = date.year_month_day();

  char buffer[30];
  char* ptr = buffer + 30;
  *--ptr = '\0';
  unsigned index = duration.seconds() * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  *--ptr = ':';
  index = duration.minutes() * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  *--ptr = ':';
  index = duration.hours() * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  *--ptr = ' ';
  index = ymd.day * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  *--ptr = '-';
  index = ymd.month * 3;
  *--ptr = months[index + 2];
  *--ptr = months[index + 1];
  *--ptr = months[index];
  *--ptr = '-';
  index = (ymd.year % 100) * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  index = (ymd.year / 100) * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];

  std::string ret(ptr);

  auto frac_sec = duration.fractional_seconds();
  if (frac_sec != 0)
  {
    std::string fmt = ",%0" + Fmi::to_string(duration.num_fractional_digits()) + "ld";
    ret += fmt::sprintf(fmt, frac_sec);
  }
  return ret;
}

// Convert a valid time to form "Fri, 27 Jul 2018 11:26:04 GMT" suitable for HTTP responses
std::string to_http_string(const boost::posix_time::ptime& time)
{
  if (time.is_special())
    throw std::runtime_error(
        "Unable to format special boost::posix_time::ptime objects for HTTP responses");

  const auto& date = time.date();
  const auto& duration = time.time_of_day();
  boost::gregorian::greg_year_month_day ymd = date.year_month_day();

  char buffer[30];
  char* ptr = buffer + 30;
  *--ptr = '\0';
  *--ptr = 'T';
  *--ptr = 'M';
  *--ptr = 'G';
  *--ptr = ' ';
  unsigned index = duration.seconds() * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  *--ptr = ':';
  index = duration.minutes() * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  *--ptr = ':';
  index = duration.hours() * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  *--ptr = ' ';
  index = (ymd.year % 100) * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  index = (ymd.year / 100) * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  *--ptr = ' ';
  index = ymd.month * 3;
  *--ptr = months[index + 2];
  *--ptr = months[index + 1];
  *--ptr = months[index];
  *--ptr = ' ';
  index = ymd.day * 2;
  *--ptr = digits[index + 1];
  *--ptr = digits[index];
  *--ptr = ' ';
  *--ptr = ',';
  index = date.day_of_week() * 3;
  *--ptr = weekdays[index + 2];
  *--ptr = weekdays[index + 1];
  *--ptr = weekdays[index];
  return std::string(ptr);
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

bool looks_unsigned_int(const std::string& value)
{
  if (value.empty()) return false;
  for (const auto chr : value)
    if (chr < '0' || chr > '9') return false;
  return true;
}

bool looks_signed_int(const std::string& value)
{
  if (value.empty()) return false;
  std::size_t i = 0;
  if (value[i] == '+' || value[i] == '-')
  {
    if (value.size() == 1) return false;
    ++i;
  }
  const auto sz = value.size();
  for (; i < sz; ++i)
  {
    const auto val = value[i];
    if (val < '0' || val > '9') return false;
  }
  return true;
}

}  // namespace Fmi
