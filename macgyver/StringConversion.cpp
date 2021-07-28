#include "StringConversion.h"
#include "Exception.h"
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>  // for is_special() cases
#include <boost/numeric/conversion/cast.hpp>               // numeric_cast
#include <boost/spirit/include/qi.hpp>
#include <fmt/format.h>
#include <fmt/printf.h>
#include <array>
#include <cmath>
#include <stdexcept>

namespace
{
const char* weekdays = "SunMonTueWedThuFriSatSun";
const char* months = "   JanFebMarAprMayJunJulAugSepOctNovDec";

const std::array<char, 201> digits{
    "0001020304050607080910111213141516171819"
    "2021222324252627282930313233343536373839"
    "4041424344454647484950515253545556575859"
    "6061626364656667686970717273747576777879"
    "8081828384858687888990919293949596979899"};

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

std::string to_string(bool value)
{
  return value ? "1" : "0";
}

#if defined(_WIN32) || defined(WIN32)

std::string to_string(size_t value)
{
  try
  {
    return fmt::sprintf("%zu", value);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

std::string to_string(time_t value)
{
  try
  {
    return fmt::sprintf("%zd", value);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}
#endif

std::string to_string(float value)
{
  try
  {
    return fmt::sprintf("%g", value);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

std::string to_string(double value)
{
  try
  {
    return fmt::sprintf("%g", value);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

std::string to_string(const char* fmt, int value)
{
  try
  {
    return fmt::sprintf(fmt, value);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

std::string to_string(const char* fmt, long value)
{
  try
  {
    return fmt::sprintf(fmt, value);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

std::string to_string(const char* fmt, unsigned int value)
{
  try
  {
    return fmt::sprintf(fmt, value);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

std::string to_string(const char* fmt, unsigned long value)
{
  try
  {
    return fmt::sprintf(fmt, value);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

std::string to_string(const char* fmt, float value)
{
  try
  {
    return fmt::sprintf(fmt, value);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

std::string to_string(const char* fmt, double value)
{
  try
  {
    return fmt::sprintf(fmt, value);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

std::string to_string(int value)
{
  try
  {
    return fmt::format_int(value).str();
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

std::string to_string(unsigned int value)
{
  try
  {
    return fmt::format_int(value).str();
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

std::string to_string(long value)
{
  try
  {
    return fmt::format_int(value).str();
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

std::string to_string(unsigned long value)
{
  try
  {
    return fmt::format_int(value).str();
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

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
  try
  {
    long result;
    std::string::const_iterator begin = str.begin(), end = str.end();
    if (boost::spirit::qi::parse(begin, end, boost::spirit::qi::long_, result))
      if (begin == end)
        return boost::numeric_cast<int>(result);

    throw Fmi::Exception(BCP, "Fmi::stoi failed to convert '" + str + "' to integer");
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

long stol(const std::string& str)
{
  try
  {
    long result;
    std::string::const_iterator begin = str.begin(), end = str.end();
    if (boost::spirit::qi::parse(begin, end, boost::spirit::qi::long_, result))
      if (begin == end)
        return result;

    throw Fmi::Exception(BCP, "Fmi::stol failed to convert '" + str + "' to long");
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

unsigned long stoul(const std::string& str)
{
  try
  {
    unsigned long result;
    std::string::const_iterator begin = str.begin(), end = str.end();
    if (boost::spirit::qi::parse(begin, end, boost::spirit::qi::ulong_, result))
      if (begin == end)
        return result;

    throw Fmi::Exception(BCP, "Fmi::stoul failed to convert '" + str + "' to unsigned long");
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

float stof(const std::string& str)
{
  try
  {
    // We parse as double because of this:
    // http://stackoverflow.com/questions/17391348/boost-spirit-floating-number-parser-precision

    double result;
    std::string::const_iterator begin = str.begin(), end = str.end();
    if (boost::spirit::qi::parse(begin, end, boost::spirit::qi::double_, result))
    {
      if (begin == end)
      {
        if (std::isfinite(result))
          return boost::numeric_cast<float>(result);

        throw Fmi::Exception(BCP, "Infinite numbers are not allowed: '" + str + "' in Fmi::stof");
      }
    }
    throw Fmi::Exception(BCP, "Fmi::stof failed to convert '" + str + "' to float");
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

double stod(const std::string& str)
{
  try
  {
    double result;
    std::string::const_iterator begin = str.begin(), end = str.end();
    if (boost::spirit::qi::parse(begin, end, boost::spirit::qi::double_, result))
    {
      if (begin == end)
      {
        if (std::isfinite(result))
          return result;

        throw Fmi::Exception(BCP, "Infinite numbers are not allowed: '" + str + "' in Fmi::stod");
      }
    }
    throw Fmi::Exception(BCP, "Fmi::stod failed to convert '" + str + "' to double");
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
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
  try
  {
    if (duration.is_special())
      return boost::posix_time::to_iso_string(duration);

    std::array<char, 8> buffer;
    char* ptr = buffer.data() + buffer.size();
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
    if (duration.is_negative())
      *--ptr = '-';
    std::string ret(ptr);

    auto frac_sec = duration.fractional_seconds();
    if (frac_sec != 0)
    {
      std::string fmt = ",%0" + Fmi::to_string(duration.num_fractional_digits()) + "ld";
      ret += fmt::sprintf(fmt, frac_sec);
    }
    return ret;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// Convert a duration to string of form HH:MM:SS[,fffffff]
std::string to_simple_string(const boost::posix_time::time_duration& duration)
{
  try
  {
    if (duration.is_special())
      return boost::posix_time::to_simple_string(duration);

    std::array<char, 10> buffer;
    char* ptr = buffer.data() + buffer.size();
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
    if (duration.is_negative())
      *--ptr = '-';
    std::string ret(ptr);

    auto frac_sec = duration.fractional_seconds();
    if (frac_sec != 0)
    {
      std::string fmt = ",%0" + Fmi::to_string(duration.num_fractional_digits()) + "ld";
      ret += fmt::sprintf(fmt, frac_sec);
    }
    return ret;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

std::string to_iso_extended_string(const boost::posix_time::time_duration& duration)
{
  try
  {
    return Fmi::to_simple_string(duration);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// Convert date to form YYYY-mmm-DD string where mmm 3 char month name
std::string to_simple_string(const boost::gregorian::date& date)
{
  try
  {
    boost::gregorian::greg_year_month_day ymd = date.year_month_day();
    std::array<char, 12> buffer;
    char* ptr = buffer.data() + buffer.size();
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
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// Convert date to form YYYYMMDD
std::string to_iso_string(const boost::gregorian::date& date)
{
  try
  {
    boost::gregorian::greg_year_month_day ymd = date.year_month_day();
    std::array<char, 9> buffer;
    char* ptr = buffer.data() + buffer.size();
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
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// Convert date to form YYYY-MM-DD
std::string to_iso_extended_string(const boost::gregorian::date& date)
{
  try
  {
    boost::gregorian::greg_year_month_day ymd = date.year_month_day();
    std::array<char, 11> buffer;
    char* ptr = buffer.data() + buffer.size();
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
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// Convert to form YYYYMMDDTHHMMSS,fffffffff where T is the date-time separator
std::string to_iso_string(const boost::posix_time::ptime& time)
{
  try
  {
    if (time.is_special())
      return boost::posix_time::to_iso_string(time);

    const auto& date = time.date();
    const auto& duration = time.time_of_day();
    boost::gregorian::greg_year_month_day ymd = date.year_month_day();

    std::array<char, 16> buffer;
    char* ptr = buffer.data() + buffer.size();
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
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// Convert to form YYYYMMDDTHHMMSS where T is the date-time separator
std::string to_iso_string(const std::time_t time)
{
  try
  {
    struct tm tt;
    gmtime_r(&time, &tt);

    std::array<char, 16> buffer;
    char* ptr = buffer.data() + buffer.size();
    *--ptr = '\0';
    unsigned index = tt.tm_sec * 2;
    *--ptr = digits[index + 1];
    *--ptr = digits[index];
    index = tt.tm_min * 2;
    *--ptr = digits[index + 1];
    *--ptr = digits[index];
    index = tt.tm_hour * 2;
    *--ptr = digits[index + 1];
    *--ptr = digits[index];
    *--ptr = 'T';
    index = tt.tm_mday * 2;
    *--ptr = digits[index + 1];
    *--ptr = digits[index];
    index = (tt.tm_mon + 1) * 2;
    *--ptr = digits[index + 1];
    *--ptr = digits[index];
    auto yy = tt.tm_year + 1900;
    index = (yy % 100) * 2;
    *--ptr = digits[index + 1];
    *--ptr = digits[index];
    index = (yy / 100) * 2;
    *--ptr = digits[index + 1];
    *--ptr = digits[index];

    return std::string(ptr);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// Convert to form YYYYMMDDTHHMM
std::string to_timestamp_string(const boost::posix_time::ptime& time)
{
  try
  {
    if (time.is_special())
      return boost::posix_time::to_iso_string(time);

    const auto& date = time.date();
    const auto& duration = time.time_of_day();
    boost::gregorian::greg_year_month_day ymd = date.year_month_day();

    std::array<char, 13> buffer;
    char* ptr = buffer.data() + buffer.size();
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
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// Convert to form YYYY-MM-DDTHH:MM:SS,fffffffff where T is the date-time separator
std::string to_iso_extended_string(const boost::posix_time::ptime& time)
{
  try
  {
    if (time.is_special())
      return boost::posix_time::to_iso_extended_string(time);

    const auto& date = time.date();
    const auto& duration = time.time_of_day();
    boost::gregorian::greg_year_month_day ymd = date.year_month_day();

    std::array<char, 20> buffer;
    char* ptr = buffer.data() + buffer.size();
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
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// Convert to form YYYY-mmm-DD HH:MM:SS.fffffffff string where mmm 3 char month name
std::string to_simple_string(const boost::posix_time::ptime& time)
{
  try
  {
    if (time.is_special())
      return boost::posix_time::to_simple_string(time);

    const auto& date = time.date();
    const auto& duration = time.time_of_day();
    boost::gregorian::greg_year_month_day ymd = date.year_month_day();

    std::array<char, 30> buffer;
    char* ptr = buffer.data() + buffer.size();
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
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// Convert a valid time to form "Fri, 27 Jul 2018 11:26:04 GMT" suitable for HTTP responses
std::string to_http_string(const boost::posix_time::ptime& time)
{
  try
  {
    if (time.is_special())
      throw Fmi::Exception(
          BCP, "Unable to format special boost::posix_time::ptime objects for HTTP responses");

    const auto& date = time.date();
    const auto& duration = time.time_of_day();
    boost::gregorian::greg_year_month_day ymd = date.year_month_day();

    std::array<char, 30> buffer;
    char* ptr = buffer.data() + buffer.size();
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
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// Convert to lower case with ASCII input
void ascii_tolower(std::string& input)
{
  try
  {
    for (auto& chr : input)
    {
      if (chr > 64 && chr < 91)
        chr += 32;
    }
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// Convert to upper case with ASCII input
void ascii_toupper(std::string& input)
{
  try
  {
    for (auto& chr : input)
    {
      if (chr > 96 && chr < 123)
        chr -= 32;
    }
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ASCII lower case with copy
std::string ascii_tolower_copy(std::string input)
{
  try
  {
    ascii_tolower(input);
    return input;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ASCII upper case with copy
std::string ascii_toupper_copy(std::string input)
{
  try
  {
    ascii_toupper(input);
    return input;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

bool looks_unsigned_int(const std::string& value)
{
  try
  {
    if (value.empty())
      return false;

    for (const auto chr : value)
      if (chr < '0' || chr > '9')
        return false;

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

bool looks_signed_int(const std::string& value)
{
  try
  {
    if (value.empty())
      return false;

    std::size_t i = 0;
    if (value[i] == '+' || value[i] == '-')
    {
      if (value.size() == 1)
        return false;
      ++i;
    }
    const auto sz = value.size();
    for (; i < sz; ++i)
    {
      const auto val = value[i];
      if (val < '0' || val > '9')
        return false;
    }
    return true;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// In place trim
void trim(std::string& value)
{
  try
  {
    const char* spaces = " \t\n\v\f\r";
    value.erase(value.find_last_not_of(spaces) + 1);
    value.erase(0, value.find_first_not_of(spaces));
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// Trim to new copy
std::string trim_copy(const std::string& value)
{
  try
  {
    auto tmp = value;
    trim(tmp);
    return tmp;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace Fmi
