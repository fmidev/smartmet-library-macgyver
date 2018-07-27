#include "StringConversion.h"

#include <fmt/format.h>
#if defined(_WIN32) || defined(WIN32)
#include <fmt/printf.h>
#endif

#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/numeric/conversion/cast.hpp>  // numeric_cast
#include <boost/spirit/include/qi.hpp>

#include <cmath>
#include <stdexcept>

namespace
{
const char* weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

const char* months[] = {
    "", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

/* Quick access table for minutes, hours, seconds, days and months in %02d format. Even though
 * libfmt does this fast, it still usually has to parse the format string. This table can be used to
 * avoid that step.
 */

const char* ints_02d[] = {
    "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "10", "11", "12", "13", "14",
    "15", "16", "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29",
    "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "40", "41", "42", "43", "44",
    "45", "46", "47", "48", "49", "50", "51", "52", "53", "54", "55", "56", "57", "58", "59",
    "60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "70", "71", "72", "73", "74",
    "75", "76", "77", "78", "79", "80", "81", "82", "83", "84", "85", "86", "87", "88", "89",
    "90", "91", "92", "93", "94", "95", "96", "97", "98", "99"};

// Years 1900-2100 optimized in similar fashion.

const char* years_04d[] = {
    "1900", "1901", "1902", "1903", "1904", "1905", "1906", "1907", "1908", "1909", "1910", "1911",
    "1912", "1913", "1914", "1915", "1916", "1917", "1918", "1919", "1920", "1921", "1922", "1923",
    "1924", "1925", "1926", "1927", "1928", "1929", "1930", "1931", "1932", "1933", "1934", "1935",
    "1936", "1937", "1938", "1939", "1940", "1941", "1942", "1943", "1944", "1945", "1946", "1947",
    "1948", "1949", "1950", "1951", "1952", "1953", "1954", "1955", "1956", "1957", "1958", "1959",
    "1960", "1961", "1962", "1963", "1964", "1965", "1966", "1967", "1968", "1969", "1970", "1971",
    "1972", "1973", "1974", "1975", "1976", "1977", "1978", "1979", "1980", "1981", "1982", "1983",
    "1984", "1985", "1986", "1987", "1988", "1989", "1990", "1991", "1992", "1993", "1994", "1995",
    "1996", "1997", "1998", "1999", "2000", "2001", "2002", "2003", "2004", "2005", "2006", "2007",
    "2008", "2009", "2010", "2011", "2012", "2013", "2014", "2015", "2016", "2017", "2018", "2019",
    "2020", "2021", "2022", "2023", "2024", "2025", "2026", "2027", "2028", "2029", "2030", "2031",
    "2032", "2033", "2034", "2035", "2036", "2037", "2038", "2039", "2040", "2041", "2042", "2043",
    "2044", "2045", "2046", "2047", "2048", "2049", "2050", "2051", "2052", "2053", "2054", "2055",
    "2056", "2057", "2058", "2059", "2060", "2061", "2062", "2063", "2064", "2065", "2066", "2067",
    "2068", "2069", "2070", "2071", "2072", "2073", "2074", "2075", "2076", "2077", "2078", "2079",
    "2080", "2081", "2082", "2083", "2084", "2085", "2086", "2087", "2088", "2089", "2090", "2091",
    "2092", "2093", "2094", "2095", "2096", "2097", "2098", "2099"};

void append_year(std::string& result, int year)
{
  if (year >= 1900 && year < 2100)
    result += years_04d[year - 1900];
  else
    result += fmt::sprintf("%04ld", year);
}

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
    if (duration.is_not_a_date_time()) return "not-a-date-time";
    if (duration.is_pos_infinity()) return "+infinity";
    if (duration.is_neg_infinity()) return "-infinity";

    return "";  // this case does not exist in boost 1.59
  }
  else
  {
    std::string ret;
    ret.reserve(1 + 6 + 1);  // sign + hhmmss + terminator byte, hoping for no fractional part
    if (duration.is_negative()) ret += '-';
    ret += ints_02d[duration.hours()];
    ret += ints_02d[duration.minutes()];
    ret += ints_02d[duration.seconds()];
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
    if (duration.is_not_a_date_time()) return "not-a-date-time";
    if (duration.is_pos_infinity()) return "+infinity";
    if (duration.is_neg_infinity()) return "-infinity";

    return "";  // this case does not exist in boost 1.59
  }
  else
  {
    std::string ret;
    ret.reserve(1 + 8 + 1);  // sign + hh:mm:ss + terminator byte, hoping for no fractional part
    if (duration.is_negative()) ret += '-';
    ret += ints_02d[duration.hours()];
    ret += ":";
    ret += ints_02d[duration.minutes()];
    ret += ":";
    ret += ints_02d[duration.seconds()];
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
  std::string ret;
  ret.reserve(4 + 2 + 2);
  append_year(ret, ymd.year);
  ret += ints_02d[ymd.month];
  ret += ints_02d[ymd.day];
  return ret;
}

// Convert date to form YYYY-MM-DD
std::string to_iso_extended_string(const boost::gregorian::date& date)
{
  boost::gregorian::greg_year_month_day ymd = date.year_month_day();
  std::string ret;
  ret.reserve(4 + 1 + 2 + 1 + 2);
  append_year(ret, ymd.year);
  ret += '-';
  ret += ints_02d[ymd.month];
  ret += '-';
  ret += ints_02d[ymd.day];
  return ret;
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

// Convert a valid time to form "Fri, 27 Jul 2018 11:26:04 GMT" suitable for HTTP responses
std::string to_http_string(const boost::posix_time::ptime& time)
{
  const auto& date = time.date();
  const auto& duration = time.time_of_day();
  std::string ret;
  ret.reserve(30);
  ret += weekdays[date.day_of_week()];
  ret += ", ";
  ret += ints_02d[date.day()];
  ret += ' ';
  ret += months[date.month()];
  ret += ' ';
  append_year(ret, date.year());
  ret += ' ';
  ret += ints_02d[duration.hours()];
  ret += ':';
  ret += ints_02d[duration.minutes()];
  ret += ':';
  ret += ints_02d[duration.seconds()];
  ret += " GMT";
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
