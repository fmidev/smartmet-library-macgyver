#include "Cast.h"
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <boost/format.hpp>

using boost::str;
using boost::format;

static const char *fmt_invalid_as = "Fmi::%1%: string '%3%' is invalid as %2% number";

static const char *fmt_garbage_after =
    "Fmi::%1%: non space characters found"
    " after %2% number in '%3%': '%4%'";

static const char *fmt_range_error = "Fmi::%1%: %2% number %3% is out of range %4%..%5%";

int64_t Fmi::str2int(const std::string &text, int64_t lower_limit, int64_t upper_limit)
{
  int n, pos;
  signed long long result;
  const char *w = text.c_str();

  n = sscanf(w, "%lld%n", &result, &pos);
  if (n != 1)
  {
    boost::basic_format<char> fmt(fmt_invalid_as);
    throw std::invalid_argument(str(fmt % __FUNCTION__ % "integer" % text));
  }

  for (w += pos; *w && isspace(*w); w++)
  {
  }

  if (*w)
  {
    boost::basic_format<char> fmt(fmt_garbage_after);
    throw std::invalid_argument(str(fmt % __FUNCTION__ % "integer" % text % w));
  }

  if ((result < lower_limit) || (result > upper_limit))
  {
    boost::basic_format<char> fmt(fmt_range_error);
    throw std::out_of_range(
        str(fmt % __FUNCTION__ % "integer" % result % lower_limit % upper_limit));
  }

  return result;
}

uint64_t Fmi::str2uint(const std::string &text, uint64_t lower_limit, uint64_t upper_limit)
{
  int n, pos;
  unsigned long long result;
  const char *w = text.c_str();

  // Under Linux sscanf reads "-1" with format %llu without errors.
  // So check additionally whether string does not contain '-'.
  if (strchr(w, '-') != NULL)
  {
    boost::basic_format<char> fmt(fmt_invalid_as);
    throw std::invalid_argument(str(fmt % __FUNCTION__ % "unsigned integer" % text));
  }

  n = sscanf(w, "%llu%n", &result, &pos);
  if (n != 1)
  {
    boost::basic_format<char> fmt(fmt_invalid_as);
    throw std::invalid_argument(str(fmt % __FUNCTION__ % "unsigned integer" % text));
  }

  for (w += pos; *w && isspace(*w); w++)
  {
  }

  if (*w)
  {
    boost::basic_format<char> fmt(fmt_garbage_after);
    throw std::invalid_argument(str(fmt % __FUNCTION__ % "unsigned integer" % text % w));
  }

  if ((result < lower_limit) || (result > upper_limit))
  {
    boost::basic_format<char> fmt(fmt_range_error);
    throw std::out_of_range(
        str(fmt % __FUNCTION__ % "integer" % result % lower_limit % upper_limit));
  }

  return result;
}

double Fmi::str2double(const std::string &text, double lower_limit, double upper_limit)
{
  int n, pos;
  double result;
  const char *w = text.c_str();

  n = std::sscanf(w, "%lf%n", &result, &pos);
  if (n != 1)
  {
    boost::basic_format<char> fmt(fmt_invalid_as);
    throw std::invalid_argument(str(fmt % __FUNCTION__ % "double" % text));
  }

  for (w += pos; *w && isspace(*w); w++)
  {
  }

  if (*w)
  {
    boost::basic_format<char> fmt(fmt_garbage_after);
    throw std::invalid_argument(str(fmt % __FUNCTION__ % "double" % text % w));
  }

  if ((result < lower_limit) || (result > upper_limit))
  {
    boost::basic_format<char> fmt(fmt_range_error);
    throw std::out_of_range(
        str(fmt % __FUNCTION__ % "integer" % result % lower_limit % upper_limit));
  }

  return result;
}
