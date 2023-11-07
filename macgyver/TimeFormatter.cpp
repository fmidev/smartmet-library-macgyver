// ======================================================================
/*!
 * \brief Format boost time objects
 */
// ======================================================================

#include "TimeFormatter.h"
#include "Exception.h"
#include "StringConversion.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <fmt/format.h>
#include <fmt/printf.h>
#include <array>
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
  try
  {
#if 0
    // THIS IS NOT THREAD SAFE IF LOCALTIME_R IS CALLED SIMULTANEOUSLY!!!
    return ::timegm(t);  // timegm is a GNU extension

#else  // Windows
    const int MINUTE = 60;
    const int HOUR = 60 * MINUTE;
    const int DAY = 24 * HOUR;
    const int YEAR = 365 * DAY;

    const std::array<int, 12> mon{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (t->tm_year < 70)
      return (static_cast<time_t>(-1));

    int n = t->tm_year + 1900 - 1;
    time_t epoch = (t->tm_year - 70) * YEAR +
                   ((n / 4 - n / 100 + n / 400) - (1969 / 4 - 1969 / 100 + 1969 / 400)) * DAY;

    int y = t->tm_year + 1900;
    int m = 0;
    for (int i = 0; i < t->tm_mon; i++)
    {
      epoch += mon[m] * DAY;
      if (m == 1 && y % 4 == 0 && (y % 100 != 0 || y % 400 == 0))
        epoch += DAY;
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
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

namespace Fmi
{
// ----------------------------------------------------------------------
/*!
 * \brief Destructor defined here to prevent existence of a weak vtable
 */
// ----------------------------------------------------------------------

TimeFormatter::~TimeFormatter() = default;
// ----------------------------------------------------------------------
/*!
 * \brief ISO-formatter (see boost manuals)
 *
 * Convert to form YYYYMMDDTHHMMSS,fffffffff where T is the date-time separator
 */
// ----------------------------------------------------------------------

struct IsoFormatter : public TimeFormatter
{
  IsoFormatter() = default;
  std::string format(const DateTime& t) const override;
  std::string format(const Fmi::LocalDateTime& t) const override;
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
  SqlFormatter() = default;
  std::string format(const DateTime& t) const override;
  std::string format(const Fmi::LocalDateTime& t) const override;
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
  XmlFormatter() = default;
  std::string format(const DateTime& t) const override;
  std::string format(const Fmi::LocalDateTime& t) const override;
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
  EpochFormatter() = default;
  std::string format(const DateTime& t) const override;
  std::string format(const Fmi::LocalDateTime& t) const override;
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
  TimeStampFormatter() = default;
  std::string format(const DateTime& t) const override;
  std::string format(const Fmi::LocalDateTime& t) const override;
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
  HttpFormatter() = default;
  std::string format(const DateTime& t) const override;
  std::string format(const Fmi::LocalDateTime& t) const override;
};

// ----------------------------------------------------------------------
/*!
 * \brief Format a ptime
 */
// ----------------------------------------------------------------------

std::string IsoFormatter::format(const DateTime& t) const
{
  try
  {
    return Fmi::to_iso_string(t);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Format a local date time
 */
// ----------------------------------------------------------------------

std::string IsoFormatter::format(const Fmi::LocalDateTime& t) const
{
  try
  {
    return Fmi::to_iso_string(t.local_time());
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Format a ptime
 */
// ----------------------------------------------------------------------

std::string SqlFormatter::format(const DateTime& t) const
{
  try
  {
    std::string tmp = Fmi::to_iso_extended_string(t);
    tmp[10] = ' ';
    return tmp;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Format a local date time
 */
// ----------------------------------------------------------------------

std::string SqlFormatter::format(const Fmi::LocalDateTime& t) const
{
  try
  {
    std::string tmp = Fmi::to_iso_extended_string(t.local_time());
    tmp[10] = ' ';
    return tmp;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Format a ptime
 */
// ----------------------------------------------------------------------

std::string XmlFormatter::format(const DateTime& t) const
{
  try
  {
    return Fmi::to_iso_extended_string(t);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Format a local date time
 */
// ----------------------------------------------------------------------

std::string XmlFormatter::format(const Fmi::LocalDateTime& t) const
{
  try
  {
    return Fmi::to_iso_extended_string(t.local_time());
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Format a ptime
 */
// ----------------------------------------------------------------------

std::string EpochFormatter::format(const DateTime& t) const
{
  try
  {
    tm tmp = boost::posix_time::to_tm(t);
    time_t epo = ::my_timegm(&tmp);
    return Fmi::to_string(epo);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Format a local date time
 */
// ----------------------------------------------------------------------

std::string EpochFormatter::format(const Fmi::LocalDateTime& t) const
{
  try
  {
    tm tmp = boost::posix_time::to_tm(t.utc_time());
    time_t epo = ::my_timegm(&tmp);
    return Fmi::to_string(epo);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Format a ptime: YYYYMMDDHHMI
 */
// ----------------------------------------------------------------------

std::string TimeStampFormatter::format(const DateTime& t) const
{
  try
  {
    return Fmi::to_timestamp_string(t);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Format a local date time
 */
// ----------------------------------------------------------------------

std::string TimeStampFormatter::format(const Fmi::LocalDateTime& t) const
{
  try
  {
    return Fmi::to_timestamp_string(t.local_time());
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Format a ptime into style Sun, 06 Nov 1994 08:49:37 GMT
 */
// ----------------------------------------------------------------------

std::string HttpFormatter::format(const DateTime& t) const
{
  try
  {
    return Fmi::to_http_string(t);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Format a local date time
 */
// ----------------------------------------------------------------------

std::string HttpFormatter::format(const Fmi::LocalDateTime& t) const
{
  try
  {
    return format(t.utc_time());
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Create a time formatter
 */
// ----------------------------------------------------------------------

TimeFormatter* TimeFormatter::create(const std::string& name)
{
  try
  {
    if (name == "iso")
      return new IsoFormatter();
    if (name == "sql")
      return new SqlFormatter();
    if (name == "xml")
      return new XmlFormatter();
    if (name == "epoch")
      return new EpochFormatter();
    if (name == "timestamp")
      return new TimeStampFormatter();
    if (name == "http")
      return new HttpFormatter();

    throw Fmi::Exception(BCP, "Unknown time format '" + name + "'");
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace Fmi

// ======================================================================
