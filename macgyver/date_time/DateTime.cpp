#include "DateTime.h"
#include "../Exception.h"
#include "../StringConversion.h"

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
    try
    {
        return Fmi::date_time::parse_date_time(" %Y-%b-%d %H:%M:%S", src);
    }
    catch(...)
    {
        throw Fmi::Exception::Trace(BCP, "Operation failed!");
    }
}    

Fmi::date_time::DateTime Fmi::date_time::parse_date_time(
    const std::string& format,
    const std::string& str)
{
    try
    {
        const auto handle_remainder =
            [](const std::string src, const std::string& remainder) -> std::string
            {
                const auto rLen = remainder.length();
                const auto srcLen = src.length();
                const std::string before = src.substr(0, srcLen - rLen);
                return before + "^^^" + remainder;
            };


        detail::time_point_t result;
        if (str.empty())
            return result;
        std::istringstream is(str);
        is.exceptions(std::ios_base::failbit | std::ios_base::badbit);
        std::exception_ptr eptr;
        try
        {
            is >> DateTimeNS::parse(format, result);
        }
        catch (...)
        {
            eptr = std::current_exception();
        }
        std::string remaining;
        std::copy(std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>(),
            std::back_inserter(remaining));

        if (eptr)
        {
            try
            {
                std::rethrow_exception(eptr);
            }
            catch (...)
            {
                auto err = Fmi::Exception::Trace(BCP,
                    "Failed to parse date from string '" + str + "'");
                err.addParameter("Error position", "'" + handle_remainder(str, remaining) + "'");
                throw err;
            }
        }
        else
        {
            // There may be more digit afer supported 6 - skip them
            std::size_t pos = remaining.find_first_not_of("0123456789");
            if (pos != std::string::npos)
                remaining = remaining.substr(pos);

            if (!Fmi::trim_copy(remaining).empty())
            {
                auto err = Fmi::Exception::Trace(BCP,
                    "DateTime parse of string '" + str + "' failed (unexpected data after DateTime)");
                err.addParameter("Error position", "'" + handle_remainder(str, remaining) + "'");
                throw err;
            }
        }
        return DateTime(result);
    }
    catch (...)
    {
        throw Fmi::Exception::Trace(BCP, "Operation failed!");
    }
}

std::ostream& Fmi::date_time::operator<<(std::ostream& os, const DateTime& dt)
{
    os << dt.as_string();
    return os;
}
