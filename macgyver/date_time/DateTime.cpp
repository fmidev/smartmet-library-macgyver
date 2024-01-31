#include "DateTime.h"
#include "../Exception.h"
#include "../StringConversion.h"

namespace
{
    struct StreamExceptionState
    {
        StreamExceptionState(std::istream& is)
            : is(is)
            , state(is.exceptions())
        {
        }

        ~StreamExceptionState()
        {
            is.exceptions(state);
        }

        std::istream& is;
        std::ios::iostate state;
    };
}

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

Fmi::date_time::Date Fmi::date_time::DateTime::date() const
{
    if (is_special())
        throw Fmi::Exception(BCP, "Cannot get date from special DateTime");
    return Date(DateTimeNS::floor<detail::days_t>(m_time_point));
}

Fmi::date_time::TimeDuration Fmi::date_time::DateTime::time_of_day() const
{
    if (is_special())
        throw Fmi::Exception(BCP, "Cannot get time of day from special DateTime");
    return TimeDuration(m_time_point - DateTimeNS::floor<detail::days_t>(m_time_point));
}

std::string Fmi::date_time::DateTime::as_string() const
{
    if (is_special())
        return Base::as_string();
    return date().as_string() + " " + time_of_day().as_string();
}

std::string Fmi::date_time::DateTime::as_iso_string() const
{
    if (is_special())
        return Base::as_string();
    return date().as_iso_string() + "T" + time_of_day().as_iso_string();
}

std::string Fmi::date_time::DateTime::as_iso_extended_string() const
{
    if (is_special())
        return Base::as_string();
    return date().as_iso_extended_string() + "T" + time_of_day().as_iso_extended_string();
}

Fmi::date_time::DateTime Fmi::date_time::DateTime::from_stream(std::istream& is, bool assume_eoi)
{
    const StreamExceptionState save(is);
    is.exceptions(std::ios::failbit | std::ios::badbit);

    Fmi::date_time::Date date;
    Fmi::date_time::TimeDuration time;

    try
    {
        is >> std::ws;
        date = Fmi::date_time::Date::from_stream(is, false);
        if (is.eof())
        {
            time = Fmi::date_time::TimeDuration(0, 0, 0);
        }
        else
        {
            time = Fmi::date_time::TimeDuration::from_stream(is);
        }
    }
    catch(const std::exception& e)
    {
        auto err = Fmi::Exception::Trace(BCP, "Failed to parse date from string");
        throw err;
    }

    int ws_count = 0;
    for (; (!is.eof() && std::isspace(is.peek())); ws_count++)
        is.get();

    if (!is.eof())
    {
        if (assume_eoi || ws_count == 0)
            throw Fmi::Exception(BCP, "Unexpected trailing characters after date string");
    }

    return DateTime(date, time);
}

std::string Fmi::date_time::to_simple_string(const DateTime& dt)
{
    return dt.as_string();
}

std::string Fmi::date_time::to_iso_string(const DateTime& dt)
{
    return dt.as_iso_string();
}

std::string Fmi::date_time::to_iso_extended_string(const DateTime& dt)
{
    return dt.as_iso_extended_string();
}

Fmi::date_time::DateTime Fmi::date_time::time_from_string(const std::string& src)
{
    std::istringstream is(src);
    try
    {
        return Fmi::date_time::DateTime::from_stream(is, true);
    }
    catch(...)
    {
        auto err = Fmi::Exception::Trace(BCP, "Operation failed!");
        err.addParameter("Error position", "'" + Fmi::detail::handle_parse_remainder(is) + "'");
        throw err;
    }
}    

std::ostream& Fmi::date_time::operator<<(std::ostream& os, const DateTime& dt)
{
    os << dt.as_string();
    return os;
}
