#include "TimeDuration.h"
#include "../Exception.h"
#include <boost/regex.hpp>

Fmi::date_time::TimeDuration::TimeDuration(const detail::duration_t& duration)
    : Fmi::date_time::Base(Fmi::date_time::Base::NORMAL)
    , m_duration(duration)
{
}

Fmi::date_time::TimeDuration::TimeDuration(int hours, int minutes, int seconds, int microseconds)
    : Fmi::date_time::Base(Fmi::date_time::Base::NORMAL)
    , m_duration(
          detail::hours_t(hours) +
          detail::minutes_t(minutes) +
          detail::seconds_t(seconds) +
          detail::microsec_t(microseconds))
{
}

int64_t Fmi::date_time::TimeDuration::hours() const
{
  assert_special();
  return std::chrono::duration_cast<detail::hours_t>(m_duration).count();
}

int64_t Fmi::date_time::TimeDuration::minutes() const
{
  assert_special();
  int64_t minutes = std::chrono::duration_cast<detail::minutes_t>(m_duration).count();
  int64_t hours = minutes / 60;
  return (int)(minutes - hours * 60);
}

int64_t Fmi::date_time::TimeDuration::seconds() const
{
  assert_special();
  int64_t seconds = std::chrono::duration_cast<detail::seconds_t>(m_duration).count();
  int64_t minutes = seconds / 60;
  return (int)(seconds - minutes * 60);
}

int64_t Fmi::date_time::TimeDuration::fractional_seconds() const
{
  int64_t mks = total_microseconds();
  int64_t seconds = mks / 1000000;
  return (int)(mks - seconds * 1000000);
}

int64_t Fmi::date_time::TimeDuration::total_seconds() const
{
  assert_special();
  return std::chrono::duration_cast<detail::seconds_t>(m_duration).count();
}

int64_t Fmi::date_time::TimeDuration::total_milliseconds() const
{
  assert_special();
  return std::chrono::duration_cast<detail::millisec_t>(m_duration).count();
}

int64_t Fmi::date_time::TimeDuration::total_microseconds() const
{
  assert_special();
  return std::chrono::duration_cast<detail::microsec_t>(m_duration).count();
}

std::string Fmi::date_time::TimeDuration::as_string() const
{
  if (is_special())
    return Fmi::date_time::Base::as_string();

  const std::string str = DateTimeNS::format("%H:%M:%S", m_duration);
  const std::size_t pos = str.find_last_of(".,");
  if (pos != std::string::npos && str.substr(pos + 1) == "000000")
    return str.substr(0, pos);

  return str;
}

std::string Fmi::date_time::TimeDuration::as_iso_string() const
{
  if (is_special())
    return Fmi::date_time::Base::as_string();

  const std::string str = DateTimeNS::format("%H%M%S", m_duration);
  const std::size_t pos = str.find_last_of(".,");
  if (pos != std::string::npos && str.substr(pos + 1) == "000000")
    return str.substr(0, pos);

  return str;
}

void Fmi::date_time::TimeDuration::assert_special() const
{
  if (is_special())
    throw Fmi::Exception(BCP, "Cannot perform operation on special time duration");
}

Fmi::date_time::TimeDuration Fmi::date_time::hours(int hours)
{
  return Fmi::date_time::TimeDuration(detail::hours_t(hours));
}

Fmi::date_time::TimeDuration Fmi::date_time::minutes(int minutes)
{
  return Fmi::date_time::TimeDuration(detail::minutes_t(minutes));
}

Fmi::date_time::TimeDuration Fmi::date_time::seconds(int seconds)
{
  return Fmi::date_time::TimeDuration(detail::seconds_t(seconds));
}

Fmi::date_time::TimeDuration Fmi::date_time::milliseconds(int milliseconds)
{
  return Fmi::date_time::TimeDuration(detail::millisec_t(milliseconds));
}

Fmi::date_time::TimeDuration Fmi::date_time::microseconds(int microseconds)
{
  return Fmi::date_time::TimeDuration(detail::microsec_t(microseconds));
}


bool Fmi::date_time::TimeDuration::operator==(const TimeDuration& other) const
{
  if (is_special() || other.is_special())
    return Base::operator==(other);

  return m_duration == other.m_duration;
}

bool Fmi::date_time::TimeDuration::operator!=(const TimeDuration& other) const
{
  if (is_special() || other.is_special())
    return Base::operator!=(other);

  return m_duration != other.m_duration;
}

bool Fmi::date_time::TimeDuration::operator<(const TimeDuration& other) const
{
  if (is_special() || other.is_special())
    return Base::operator<(other);

  return m_duration < other.m_duration;
}

bool Fmi::date_time::TimeDuration::operator<=(const TimeDuration& other) const
{
  if (is_special() || other.is_special())
    return Base::operator<=(other);

  return m_duration <= other.m_duration;
}

bool Fmi::date_time::TimeDuration::operator>(const TimeDuration& other) const
{
  if (is_special() || other.is_special())
    return Base::operator>(other);

  return m_duration > other.m_duration;
}

bool Fmi::date_time::TimeDuration::operator>=(const TimeDuration& other) const
{
  if (is_special() || other.is_special())
    return Base::operator>=(other);

  return m_duration >= other.m_duration;
}

Fmi::date_time::TimeDuration Fmi::date_time::TimeDuration::operator+(const TimeDuration& other) const
{
  if (is_special() && other.is_special())
    return TimeDuration(NOT_A_DATE_TIME);

  return TimeDuration(m_duration + other.m_duration);
}

Fmi::date_time::TimeDuration Fmi::date_time::TimeDuration::operator-(const TimeDuration& other) const
{
  if (is_special() && other.is_special())
    return TimeDuration(NOT_A_DATE_TIME);

  return TimeDuration(m_duration - other.m_duration);
}

Fmi::date_time::TimeDuration Fmi::date_time::TimeDuration::operator*(int64_t factor) const
{
  if (is_special())
    return *this;

  return TimeDuration(m_duration * factor);
}

Fmi::date_time::TimeDuration Fmi::date_time::TimeDuration::operator/(int64_t factor) const
{
  if (is_special())
    return *this;

  return TimeDuration(m_duration / factor);
}

Fmi::date_time::TimeDuration
Fmi::date_time::duration_from_string(const std::string& str)
{
    static const boost::regex expr(
        "^\\s*"
        "(\\d{1,2})" // hours
        ":"
        "(\\d{1,2})" // minutes
        ":"
        "(\\d{1,2})" // seconds
        "(?:"
        "[.,]"
        "(\\d*)" // microseconds
        ")?"
        "\\s*$");

    boost::smatch match;
    if (!boost::regex_match(str, match, expr))
        throw Fmi::Exception(BCP, "Invalid time duration: '" + str + "'");

    const int hours = std::stoi(match[1]);

    const int minutes = std::stoi(match[2]);
    if (minutes < 0 || minutes > 59)
        throw Fmi::Exception(BCP, "Invalid time duration: '" + str + "'");

    const int seconds = std::stoi(match[3]);
    if (seconds < 0 || seconds > 59)
        throw Fmi::Exception(BCP, "Invalid time duration: '" + str + "'");

    std::string frac_str = match[4];
    int microseconds = 0;
    if (!frac_str.empty())
    {
        const std::size_t len = frac_str.size();
        if (len > 6)
            frac_str = frac_str.substr(0, 6);

        microseconds = std::stoi(frac_str);
        for (std::size_t i = len; i < 6; i++)
            microseconds *= 10;
    }

    return Fmi::date_time::TimeDuration(
        hours, minutes, seconds, microseconds);
}

struct std::tm Fmi::date_time::to_tm(const date_time::TimeDuration& t)
{
  if (t.is_special())
    throw Fmi::Exception(BCP, "Cannot convert special time to tm");

  struct std::tm result;
  result.tm_year = 0;
  result.tm_mon = 0;
  result.tm_mday = 0;
  result.tm_hour = t.hours();
  result.tm_min = t.minutes();
  result.tm_sec = t.seconds();
  result.tm_isdst = -1;
  return result;
}

std::string Fmi::date_time::to_string(const TimeDuration& td)
{
  return td.as_string();
}

std::string Fmi::date_time::to_iso_string(const TimeDuration& td)
{
  return td.as_iso_string();
}

std::ostream& Fmi::date_time::operator<<(std::ostream& os, const TimeDuration& td)
{
  return os << td.as_string();
}
