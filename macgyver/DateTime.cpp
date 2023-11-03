#include "DateTime.h"
#include <iomanip>
#include <fmt/format.h>
#include "Exception.h"

namespace
{
    std::string maybe_discard_seconds_part(std::string&& src)
    {
        const std::size_t pos = src.find_last_not_of("0");
        if (pos == std::string::npos) {
            return src;
        } else if (src[pos] == '.') {
            return std::string(src.begin(), src.begin() + pos);
        } else {
            return src;
        }
    }
}

Fmi::DateTime::DateTime()
    : initialized(false)
{
}

Fmi::DateTime::DateTime(const Fmi::detail::time_point_t& tp)
    : detail::time_point_t(tp)
    , initialized(true)
{
}

Fmi::DateTime::DateTime(const DateTime& src)
    : Fmi::detail::time_point_t(src)
    , initialized(src.initialized)
{
}

Fmi::DateTime::DateTime(const boost::posix_time::ptime& src)
{
    operator = (src);
}

Fmi::DateTime::DateTime(const Date& date, const TimeDuration& duration)
{
    bool date_defined = !date.is_special();
    bool duration_defined = !duration.is_special();
    if (!date_defined && !duration_defined)
    {
        initialized = false;
    }
    else if (date_defined && duration_defined)
    {
        operator = (boost::posix_time::ptime(date, duration));
    }
    else
    {
        std::ostringstream msg;
        msg << "Conflicting arguments '" << date << "' and '" << duration << '\'';
        throw Fmi::Exception(BCP, msg.str());
    }
}

Fmi::DateTime::~DateTime() = default;
Fmi::DateTime& Fmi::DateTime::operator = (const Fmi::DateTime& src) = default;


Fmi::DateTime&
Fmi::DateTime::operator = (const boost::posix_time::ptime& src)
{
    if (src.is_not_a_date_time())
    {
        initialized = false;
        base::operator = (detail::time_point_t());
    }
    else
    {
        using namespace detail;
        initialized = true;
        const auto date_ = src.date();
        const auto time_ = src.time_of_day();
        const date_ns::year year(date_.year());
        const date_ns::month month(date_.month());
        const date_ns::day day(date_.day());
        const date_ns::year_month_day ymd(year, month, day);
        const date_ns::local_days l_days(ymd);
        base::operator = (l_days + std::chrono::microseconds(time_.total_microseconds()));
    }
    return *this;
}

Fmi::Date
Fmi::DateTime::date() const
{
    boost::gregorian::date result;
    if (initialized)
    {
        const date_ns::year_month_day ymd(date_ns::floor<detail::days_t>(*this));
        int year = int(ymd.year());
        unsigned month = unsigned(ymd.month());
        unsigned day = unsigned(ymd.day());
        result = boost::gregorian::date(year, month, day);
    }
    return result;
}

Fmi::TimeDuration Fmi::DateTime::time_of_day() const
{
    boost::posix_time::time_duration result;
    if (initialized)
    {
        const detail::duration_t part = *this - date_ns::floor<detail::days_t>(*this);
        const int64_t sec_part = std::chrono::duration_cast<detail::seconds_t>(part).count();
        const auto hours = sec_part / 3600;
        const auto minutes = (sec_part - 3600*hours) / 60;
        const auto seconds = sec_part - 3600*hours - 60*minutes;
        const auto microseconds = (part - detail::seconds_t(sec_part)).count()
            * detail::periods_per_mks;
        result = boost::posix_time::time_duration(hours, minutes, seconds, 0);
        if (microseconds) {
            result += boost::posix_time::microseconds(microseconds);
        }
    }
    return result;
}

Fmi::DateTime::operator boost::posix_time::ptime () const
{
    boost::posix_time::ptime result;
    if (initialized)
    {
        const Date date = this->date();
        const TimeDuration duration = this->time_of_day();
        result = boost::posix_time::ptime(date, duration);
    }
    return result;
}

std::string
Fmi::DateTime::to_simple_string() const
{
    const detail::time_point_t& tpoint = *this;
    return maybe_discard_seconds_part(date_ns::format("%Y-%b-%d %H:%M:%S", tpoint));
}

std::string
Fmi::DateTime::to_iso_string() const
{
    const detail::time_point_t& tpoint = *this;
    return maybe_discard_seconds_part(date_ns::format("%Y%m%dT%H%M%S", tpoint));
}

std::string
Fmi::DateTime::to_iso_extended_string() const
{
    return maybe_discard_seconds_part(date_ns::format("%Y-%m-%dT%H:%M:%S", *this));
}

std::ostream&
Fmi::operator << (std::ostream& os, const Fmi::DateTime& src)
{
    const detail::time_point_t& tpoint = src;
    date_ns::to_stream(os, "%Y-%m-%d %H:%M:%S", tpoint);
    return os;
}

std::string
Fmi::to_simple_string(const DateTime& src)
{
    return src.to_simple_string();
}

std::string
Fmi::to_iso_string(const DateTime& src)
{
    return src.to_iso_string();
}

std::string
Fmi::to_iso_extended_string(const DateTime& src)
{
    return src.to_iso_extended_string();
}
