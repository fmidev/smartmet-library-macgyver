// ======================================================================
/*!
 * \brief Format boost time objects
 */
// ======================================================================

#include "TimeFormatter.h"
#include "StringConversion.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include <fmt/format.h>
#if defined(_WIN32) || defined(WIN32)
#include <fmt/printf.h>
#endif

#include <stdexcept>

// ----------------------------------------------------------------------
/*!
 * A local help subroutine to convert a UTC tm to UTC time_t
 * This code is copied from newbase NFmiStaticTime::my_timegm -method.
 *
 * The original C code is by C.A. Lademann and Richard Kettlewell.
 *
 * \param t The UTC time as a tm struct
 * \return The UTC time as a time_t
 * \bug This has not been verified to work in SGI/Windows
 */
// ----------------------------------------------------------------------

static time_t my_timegm(struct tm* t)
{
#if 0
  // THIS IS NOT THREAD SAFE IF LOCALTIME_R IS CALLED SIMULTANEOUSLY!!!
  return ::timegm(t);  // timegm is a GNU extension

#else  // Windows
  const int MINUTE = 60;
  const int HOUR = 60 * MINUTE;
  const int DAY = 24 * HOUR;
  const int YEAR = 365 * DAY;

  const int mon[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  if (t->tm_year < 70) return (static_cast<time_t>(-1));

  int n = t->tm_year + 1900 - 1;
  time_t epoch = (t->tm_year - 70) * YEAR +
                 ((n / 4 - n / 100 + n / 400) - (1969 / 4 - 1969 / 100 + 1969 / 400)) * DAY;

  int y = t->tm_year + 1900;
  int m = 0;
  for (int i = 0; i < t->tm_mon; i++)
  {
    epoch += mon[m] * DAY;
    if (m == 1 && y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) epoch += DAY;
    if (++m > 11)
    {
      m = 0;
      y++;
    }
  }

  epoch += (t->tm_mday - 1) * DAY;
  epoch += t->tm_hour * HOUR;
  epoch += t->tm_min * MINUTE;
  epoch += t->tm_sec;

  return epoch;
#endif
}

namespace Fmi
{
// ----------------------------------------------------------------------
/*!
 * \brief Destructor defined here to prevent existence of a weak vtable
 */
// ----------------------------------------------------------------------

TimeFormatter::~TimeFormatter() {}
// ----------------------------------------------------------------------
/*!
 * \brief ISO-formatter (see boost manuals)
 *
 * Convert to form YYYYMMDDTHHMMSS,fffffffff where T is the date-time separator
 */
// ----------------------------------------------------------------------

struct IsoFormatter : public TimeFormatter
{
  IsoFormatter() : TimeFormatter() {}
  virtual std::string format(const boost::posix_time::ptime& t) const;
  virtual std::string format(const boost::local_time::local_date_time& t) const;
};

// ----------------------------------------------------------------------
/*!
 * \brief SQL-formatter
 *
 * Convert to form YYYY-MM-DD HH:MM:SS
 */
// ----------------------------------------------------------------------

struct SqlFormatter : public TimeFormatter
{
  SqlFormatter() : TimeFormatter() {}
  virtual std::string format(const boost::posix_time::ptime& t) const;
  virtual std::string format(const boost::local_time::local_date_time& t) const;
};

// ----------------------------------------------------------------------
/*!
 * \brief XML-formatter
 *
 * Convert to form YYYY-MM-DDTHH:MM:SS
 */
// ----------------------------------------------------------------------

struct XmlFormatter : public TimeFormatter
{
  XmlFormatter() : TimeFormatter() {}
  virtual std::string format(const boost::posix_time::ptime& t) const;
  virtual std::string format(const boost::local_time::local_date_time& t) const;
};

// ----------------------------------------------------------------------
/*!
 * \brief Epoch-formatter
 *
 * Convert to form ssssss, time in seconds since epoch 1970-01-01 00:00:00
 */
// ----------------------------------------------------------------------

struct EpochFormatter : public TimeFormatter
{
  EpochFormatter() : TimeFormatter() {}
  virtual std::string format(const boost::posix_time::ptime& t) const;
  virtual std::string format(const boost::local_time::local_date_time& t) const;
};

// ----------------------------------------------------------------------
/*!
 * \brief Timestamp-formatter
 *
 * Convert to form YYYYMMDDHHMM
 */
// ----------------------------------------------------------------------

struct TimeStampFormatter : public TimeFormatter
{
  TimeStampFormatter() : TimeFormatter() {}
  virtual std::string format(const boost::posix_time::ptime& t) const;
  virtual std::string format(const boost::local_time::local_date_time& t) const;
};

// ----------------------------------------------------------------------
/*!
 * \brief HTTP-date formatter
 *
 * Convert to form Sun, 06 Nov 1994 08:49:37 GMT
 */
// ----------------------------------------------------------------------

struct HttpFormatter : public TimeFormatter
{
  HttpFormatter() : TimeFormatter() {}
  virtual std::string format(const boost::posix_time::ptime& t) const;
  virtual std::string format(const boost::local_time::local_date_time& t) const;
};

// ----------------------------------------------------------------------
/*!
 * \brief Format a ptime
 */
// ----------------------------------------------------------------------

std::string IsoFormatter::format(const boost::posix_time::ptime& t) const
{
  return Fmi::to_iso_string(t);
}

// ----------------------------------------------------------------------
/*!
 * \brief Format a local date time
 */
// ----------------------------------------------------------------------

std::string IsoFormatter::format(const boost::local_time::local_date_time& t) const
{
  return Fmi::to_iso_string(t.local_time());
}

// ----------------------------------------------------------------------
/*!
 * \brief Format a ptime
 */
// ----------------------------------------------------------------------

std::string SqlFormatter::format(const boost::posix_time::ptime& t) const
{
  std::string tmp = Fmi::to_iso_extended_string(t);
  tmp[10] = ' ';
  return tmp;
}

// ----------------------------------------------------------------------
/*!
 * \brief Format a local date time
 */
// ----------------------------------------------------------------------

std::string SqlFormatter::format(const boost::local_time::local_date_time& t) const
{
  std::string tmp = Fmi::to_iso_extended_string(t.local_time());
  tmp[10] = ' ';
  return tmp;
}

// ----------------------------------------------------------------------
/*!
 * \brief Format a ptime
 */
// ----------------------------------------------------------------------

std::string XmlFormatter::format(const boost::posix_time::ptime& t) const
{
  return Fmi::to_iso_extended_string(t);
}

// ----------------------------------------------------------------------
/*!
 * \brief Format a local date time
 */
// ----------------------------------------------------------------------

std::string XmlFormatter::format(const boost::local_time::local_date_time& t) const
{
  return Fmi::to_iso_extended_string(t.local_time());
}

// ----------------------------------------------------------------------
/*!
 * \brief Format a ptime
 */
// ----------------------------------------------------------------------

std::string EpochFormatter::format(const boost::posix_time::ptime& t) const
{
  tm tmp = boost::posix_time::to_tm(t);
  time_t epo = ::my_timegm(&tmp);
  return Fmi::to_string(epo);
}

// ----------------------------------------------------------------------
/*!
 * \brief Format a local date time
 */
// ----------------------------------------------------------------------

std::string EpochFormatter::format(const boost::local_time::local_date_time& t) const
{
  tm tmp = boost::posix_time::to_tm(t.utc_time());
  time_t epo = ::my_timegm(&tmp);
  return Fmi::to_string(epo);
}

// ----------------------------------------------------------------------
/*!
 * \brief Format a ptime
 */
// ----------------------------------------------------------------------

std::string TimeStampFormatter::format(const boost::posix_time::ptime& t) const
{
  std::string tmp;
  tmp.reserve(6 + 6 + 1);
  tmp += Fmi::to_iso_string(t.date());
  tmp += Fmi::to_iso_string(t.time_of_day());
  if (tmp.size() == 12) return tmp;
  return tmp.substr(0, 12);
}

// ----------------------------------------------------------------------
/*!
 * \brief Format a local date time
 */
// ----------------------------------------------------------------------

std::string TimeStampFormatter::format(const boost::local_time::local_date_time& t) const
{
  std::string tmp;
  tmp.reserve(6 + 6 + 1);
  tmp += Fmi::to_iso_string(t.local_time().date());
  tmp += Fmi::to_iso_string(t.local_time().time_of_day());
  if (tmp.size() == 12) return tmp;
  return tmp.substr(0, 12);
}

// ----------------------------------------------------------------------
/*!
 * \brief Format a ptime into style Sun, 06 Nov 1994 08:49:37 GMT
 */
// ----------------------------------------------------------------------

std::string HttpFormatter::format(const boost::posix_time::ptime& t) const
{
  static const char* weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

  static const char* months[] = {
      "", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  return fmt::sprintf("%s, %02d %s %d %02d:%02d:%02d GMT",
                      weekdays[t.date().day_of_week()],
                      t.date().day(),
                      months[t.date().month()],
                      t.date().year(),
                      t.time_of_day().hours(),
                      t.time_of_day().minutes(),
                      t.time_of_day().seconds());
}

// ----------------------------------------------------------------------
/*!
 * \brief Format a local date time
 */
// ----------------------------------------------------------------------

std::string HttpFormatter::format(const boost::local_time::local_date_time& t) const
{
  return format(t.utc_time());
}

// ----------------------------------------------------------------------
/*!
 * \brief Create a time formatter
 */
// ----------------------------------------------------------------------

TimeFormatter* TimeFormatter::create(const std::string& name)
{
  if (name == "iso")
    return new IsoFormatter();
  else if (name == "sql")
    return new SqlFormatter();
  else if (name == "xml")
    return new XmlFormatter();
  else if (name == "epoch")
    return new EpochFormatter();
  else if (name == "timestamp")
    return new TimeStampFormatter();
  else if (name == "http")
    return new HttpFormatter();
  else
    throw std::runtime_error("Unknown time format '" + name + "'");
}

}  // namespace Fmi

// ======================================================================
