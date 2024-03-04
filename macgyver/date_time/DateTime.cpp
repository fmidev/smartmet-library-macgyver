#include "DateTime.h"
#include "Internal.h"
#include "ParserDefinitions.h"
#include "../Exception.h"
#include "../StringConversion.h"

namespace internal = Fmi::date_time::internal;

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

const Fmi::date_time::DateTime Fmi::date_time::DateTime::epoch(Fmi::date_time::Date(1970, 1, 1), Fmi::date_time::Seconds(0));

Fmi::date_time::DateTime::DateTime() = default;

Fmi::date_time::DateTime::DateTime(const Type& type)
    : Base(type)
{
}

Fmi::date_time::DateTime::DateTime(const DateTime& other) = default;

Fmi::date_time::DateTime::DateTime(const Date& date)
    : Base(DateTime::NORMAL)
{
    if (date.is_special())
        throw Fmi::Exception(BCP, "Cannot convert special date to DateTime");
    m_time_point = date.get_impl();
}

Fmi::date_time::DateTime::DateTime(const Date& date, const TimeDuration& time)
    : Base(DateTime::NORMAL)
{
    if (date.is_special() || time.is_special())
        throw Fmi::Exception(BCP, "Cannot convert special date or time duration to DateTime");
    m_time_point = date.get_impl() + time.get_impl();
}

Fmi::date_time::DateTime::DateTime(const detail::time_point_t& time_point)
    : Base(DateTime::NORMAL)
    , m_time_point(time_point)
{
}

Fmi::date_time::DateTime::~DateTime() = default;

Fmi::date_time::DateTime& Fmi::date_time::DateTime::operator=(const DateTime& other) = default;

bool Fmi::date_time::DateTime::operator==(const DateTime& other) const
{
    if (is_special() || other.is_special())
        return Base::operator==(other);

    return m_time_point == other.m_time_point;
}

bool Fmi::date_time::DateTime::operator!=(const DateTime& other) const
{
    if (is_special() || other.is_special())
        return Base::operator!=(other);

    return m_time_point != other.m_time_point;
}

bool Fmi::date_time::DateTime::operator<(const DateTime& other) const
{
    if (is_special() || other.is_special())
        return Base::operator<(other);

    return m_time_point < other.m_time_point;
}

bool Fmi::date_time::DateTime::operator<=(const DateTime& other) const
{
    if (is_special() || other.is_special())
        return Base::operator<=(other);

    return m_time_point <= other.m_time_point;
}

bool Fmi::date_time::DateTime::operator>(const DateTime& other) const
{
    if (is_special() || other.is_special())
        return Base::operator>(other);

    return m_time_point > other.m_time_point;
}

bool Fmi::date_time::DateTime::operator>=(const DateTime& other) const
{
    if (is_special() || other.is_special())
        return Base::operator>=(other);

    return m_time_point >= other.m_time_point;
}

Fmi::date_time::DateTime& Fmi::date_time::DateTime::operator+=(const TimeDuration& duration)
{
    if (is_special() || duration.is_special())
        throw Fmi::Exception(BCP, "Cannot add special TimeDuration to DateTime");
    m_time_point += duration.get_impl();
    return *this;
}

Fmi::date_time::DateTime& Fmi::date_time::DateTime::operator-=(const TimeDuration& duration)
{
    if (is_special() || duration.is_special())
        throw Fmi::Exception(BCP, "Cannot subtract special TimeDuration from DateTime");
    m_time_point -= duration.get_impl();
    return *this;
}

Fmi::date_time::DateTime Fmi::date_time::DateTime::operator+(const TimeDuration& duration) const
{
    if (is_special() || duration.is_special())
        throw Fmi::Exception(BCP, "Cannot add special TimeDuration to DateTime");
    return DateTime(m_time_point + duration.get_impl());
}

Fmi::date_time::DateTime Fmi::date_time::DateTime::operator-(const TimeDuration& duration) const
{
    if (is_special() || duration.is_special())
        throw Fmi::Exception(BCP, "Cannot subtract special TimeDuration from DateTime");
    return DateTime(m_time_point - duration.get_impl());
}

Fmi::date_time::TimeDuration Fmi::date_time::DateTime::operator-(const DateTime& other) const
{
    if (is_special() || other.is_special())
        throw Fmi::Exception(BCP, "Cannot subtract special DateTime from DateTime");
    return TimeDuration(m_time_point - other.get_impl());
}

Fmi::date_time::Date Fmi::date_time::DateTime::date() const
{
    if (is_special())
        throw Fmi::Exception(BCP, "Cannot get date from special DateTime");
    return Date(date::floor<detail::days_t>(m_time_point));
}

Fmi::date_time::TimeDuration Fmi::date_time::DateTime::time_of_day() const
{
    if (is_special())
        throw Fmi::Exception(BCP, "Cannot get time of day from special DateTime");
    return TimeDuration(m_time_point - date::floor<detail::days_t>(m_time_point));
}

std::time_t Fmi::date_time::DateTime::as_time_t() const
{
    if (is_special())
        throw Fmi::Exception(BCP, "Cannot get time of day from special DateTime");
    return std::chrono::duration_cast<Fmi::detail::seconds_t>(m_time_point - epoch.m_time_point).count();
}

struct std::tm Fmi::date_time::DateTime::as_tm() const
{
    const auto date_ = date();
    const auto ymd = date_.year_month_day();
    const auto time = time_of_day();
    struct tm result;
    result.tm_year = ymd.year - 1900;
    result.tm_mon = ymd.month - 1;
    result.tm_mday = ymd.day;
    result.tm_wday = date_.day_of_week().iso_encoding();
    result.tm_yday = date_.day_of_year() - 1;
    result.tm_hour = time.hours();
    result.tm_min = time.minutes();
    result.tm_sec = time.seconds();
    result.tm_isdst = -1;
    return result;
}

std::string Fmi::date_time::DateTime::as_string() const
{
    if (is_special())
        return Base::special_time_as_string();
    return date().as_string() + " " + maybe_discard_seconds_part(time_of_day().as_string());
}

std::string Fmi::date_time::DateTime::as_iso_string() const
{
    if (is_special())
        return Base::special_time_as_string();
    return date().as_iso_string() + "T" + maybe_discard_seconds_part(time_of_day().as_iso_string());
}

std::string Fmi::date_time::DateTime::as_iso_extended_string() const
{
    if (is_special())
        return Base::special_time_as_string();
    return date().as_iso_extended_string() + "T" + maybe_discard_seconds_part(time_of_day().as_iso_extended_string());
}

Fmi::date_time::DateTime Fmi::date_time::DateTime::from_tm(const std::tm& tm)
{
    return DateTime(Date::from_tm(tm), TimeDuration::from_tm(tm));
}

namespace
{

    using namespace boost::spirit::qi;
    using iterator = std::string::const_iterator;

    Fmi::date_time::parser::GenericDateTimeParser<iterator> generic_date_time_parser;
    Fmi::date_time::parser::IsoDateTimeParser<iterator> iso_date_time_parser;
    Fmi::date_time::parser::IsoExtendedDateTimeParser<iterator> iso_extended_date_time_parser;

    Fmi::date_time::DateTime parse_impl(
        const char* filename,     // We are not interested about this location
        int line,                 // in backtrace. Therefore use parent one
        const char* function,
        const std::string& str,
        const rule<iterator, Fmi::date_time::parser::date_time_members_t()>& rule)
    {
        const std::string input = Fmi::trim_copy(str);
        iterator first = input.begin();
        iterator last = input.end();

        Fmi::date_time::parser::date_time_members_t dt;
        if (!parse(first, last, rule >> eoi, dt))
        {
            auto err = Fmi::Exception::Trace(filename, line, function,
                "Failed to parse date time from string '" + str + "'");
            throw err;
        }

        Fmi::date_time::Date date(dt.date.year, dt.date.month, dt.date.mday);
        Fmi::date_time::TimeDuration time(
            dt.hours(),
            dt.minutes(),
            dt.seconds(),
            dt.mks());

        Fmi::date_time::DateTime tmp(date, time);

        if (dt.tz_offset)
        {
            // Convert to UTC as we do not support posix time zones
            tmp - Fmi::date_time::TimeDuration(
                dt.tz_offset->hours, dt.tz_offset->minutes, 0, 0);
        }

        return tmp;
    }

    std::optional<Fmi::date_time::DateTime>
    try_parse_impl(
        const char* filename,     // We are not interested about this location
        int line,                 // in backtrace. Therefore use parent one
        const char* function,
        const std::string& str,
        const rule<iterator, Fmi::date_time::parser::date_time_members_t()>& rule)
    {
        try
        {
            const std::string input = Fmi::trim_copy(str);
            iterator first = input.begin();
            iterator last = input.end();

            Fmi::date_time::parser::date_time_members_t dt;
            if (!parse(first, last, rule >> eoi, dt))
            {
                return std::nullopt;
            }

            Fmi::date_time::Date date(dt.date.year, dt.date.month, dt.date.mday);
            Fmi::date_time::TimeDuration time(
                dt.hours(),
                dt.minutes(),
                dt.seconds(),
                dt.mks());

            Fmi::date_time::DateTime tmp(date, time);

            if (dt.tz_offset)
            {
                // Convert to UTC as we do not support posix time zones
                tmp - Fmi::date_time::TimeDuration(
                    dt.tz_offset->hours, dt.tz_offset->minutes, 0, 0);
            }

        return tmp;
        }
        catch (...)
        {
            return std::nullopt;
        }
    }
}

Fmi::date_time::DateTime Fmi::date_time::DateTime::from_string(const std::string& str)
{
    using namespace boost::spirit::qi;
    return parse_impl(BCP, str, generic_date_time_parser >> eps);
}

Fmi::date_time::DateTime Fmi::date_time::DateTime::from_iso_string(const std::string& str)
{
    using namespace boost::spirit::qi;
    return parse_impl(BCP, str, iso_date_time_parser >> eoi);
}

Fmi::date_time::DateTime Fmi::date_time::DateTime::from_iso_extended_string(const std::string& str)
{
    using namespace boost::spirit::qi;
    return parse_impl(BCP, str, iso_extended_date_time_parser >> eoi);
}

std::optional<Fmi::date_time::DateTime>
Fmi::date_time::DateTime::try_parse_iso_string(const std::string& src)
{
    using namespace boost::spirit::qi;
    return try_parse_impl(BCP, src, iso_date_time_parser >> eoi);
}

std::optional<Fmi::date_time::DateTime>
Fmi::date_time::DateTime::try_parse_iso_extended_string(const std::string& src)
{
    using namespace boost::spirit::qi;
    return try_parse_impl(BCP, src, iso_extended_date_time_parser >> eoi);
}

std::optional<Fmi::date_time::DateTime>
Fmi::date_time::DateTime::try_parse_string(const std::string& src)
{
    using namespace boost::spirit::qi;
    return parse_impl(BCP, src, generic_date_time_parser >> eps);
}

std::ostream& Fmi::date_time::operator<<(std::ostream& os, const DateTime& dt)
{
    os << dt.as_string();
    return os;
}

Fmi::date_time::DateTime Fmi::date_time::from_time_t(long time)
{
    return Fmi::date_time::DateTime(Fmi::date_time::Date::epoch.get_impl() + Fmi::detail::seconds_t(time));
}
