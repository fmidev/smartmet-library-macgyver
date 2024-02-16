#include "Date.h"
#include "Internal.hpp"
#include "../Exception.h"
#include "../StringConversion.h"

const Fmi::date_time::Date Fmi::date_time::Date::epoch(1970, 1, 1);

namespace internal = Fmi::date_time::internal;

Fmi::date_time::Date::Date() = default;

Fmi::date_time::Date::Date(const Type& type)
    : Base(type)
{
}

Fmi::date_time::Date::Date(const Date& other) = default;

Fmi::date_time::Date::Date(int year, unsigned month, unsigned day)
    : Base(Date::NORMAL)
{
    date::year year_(year);
    date::month month_(month);
    date::day day_(day);

    date::year_month_day ymd(year_, month_, day_);
    if (! ymd.ok())
    {
        std::ostringstream err;
        err << "Invalid date " << date::format("%Y-%m-%d", ymd);
        if (! ymd.year().ok())
            err << " (year)";
        if (! ymd.month().ok())
            err << " (month)";
        if (! ymd.day().ok())
            err << " (day)";

        throw Fmi::Exception(BCP, err.str());
    }
    date = date::local_days(ymd);
}

Fmi::date_time::Date::Date(const date::local_days& date)
    : Base(Date::NORMAL)
    , date(date)
{
} 

Fmi::date_time::Date::~Date() = default;

Fmi::date_time::Date& Fmi::date_time::Date::operator=(const Date& other) = default;

int Fmi::date_time::Date::year() const
{
    assert_special();
    date::year_month_day ymd(date);
    return int(ymd.year());
}

unsigned Fmi::date_time::Date::month() const
{
    assert_special();
    date::year_month_day ymd(date);
    return unsigned(ymd.month());
}

unsigned Fmi::date_time::Date::day() const
{
    assert_special();
    date::year_month_day ymd(date);
    return unsigned(ymd.day());
}

Fmi::date_time::YMD Fmi::date_time::Date::year_month_day() const
{
    assert_special();
    date::year_month_day ymd(date);

    YMD result;
    result.year = int(ymd.year());
    result.month = unsigned(ymd.month());
    result.day = unsigned(ymd.day());
    return result;
}

Fmi::date_time::Weekday Fmi::date_time::Date::day_of_week() const
{
    assert_special();
    date::weekday wd(date);
    return wd;
}

int Fmi::date_time::Date::day_of_year() const
{
    assert_special();
    date::year year_(year());
    date::month month_(1);
    date::day day_(1);
    date::year_month_day ymd(year_, month_, day_);
    return (date - date::local_days(ymd)).count() + 1;
}

Fmi::date_time::Date Fmi::date_time::Date::end_of_month() const
{
    assert_special();
    const auto ymd1 = year_month_day();
    date::year year_(ymd1.year);
    date::month month_(ymd1.month);
    date::day day_(1);

    date::year_month ym(year_, month_);
    ym += date::months(1);
    date::year_month_day ymd(ym.year(), ym.month(), day_);
    date::local_days tmp(ymd);
    tmp -= date::days(1);
    return Date(tmp);
}

long Fmi::date_time::Date::modjulian_day() const
{
    assert_special();
    return *this - epoch + 40587;
}

long Fmi::date_time::Date::julian_day() const
{
    assert_special();
    return *this - epoch + 2440588;
}

int Fmi::date_time::Date::week_number() const
{
    // Code ported from boost (boost/date_time/gregorian/gregorian_calendar.ipp)
    const auto ymd = year_month_day();
    const auto jBegin = Date(ymd.year, 1, 1).julian_day();
    const auto jCurr = julian_day();
    const unsigned long day = (jBegin + 3) % 7;
    const unsigned long week = (jCurr + day - jBegin + 4) / 7;

    if (week >= 1 && week <= 52)
    {
        return static_cast<int>(week);
    }

    if (week == 53)
    {
        if ((day == 6) || ((day == 5) && (date::year(ymd.year).is_leap())))
        {
            return static_cast<int>(week);
        }
        else
        {
            return 1;
        }
    }
    else if (week == 0)
    {
        const auto jBegin = Date(ymd.year - 1, 1, 1).julian_day();
        const auto jCurr = julian_day();
        const unsigned long day = (jBegin + 3) % 7;
        const unsigned long week = (jCurr + day -jBegin + 4) / 7;
        return static_cast<int>(week);
    }
    else
    {
        throw Fmi::Exception(BCP, "INTERNAL ERROR: failed to get"
            " week number for " + date::format("%Y-%m-%d", date));
    }
}

std::string Fmi::date_time::Date::as_string() const
{
    if (is_special())
        return Fmi::date_time::Base::as_string();

    const std::string str = date::format("%Y-%b-%d", date);
    return str;
}

std::string Fmi::date_time::Date::as_iso_string() const
{
    if (is_special())
        return Fmi::date_time::Base::as_string();

    const std::string str = date::format("%Y%m%d", date);
    return str;
}

std::string Fmi::date_time::Date::as_iso_extended_string() const
{
    if (is_special())
        return Fmi::date_time::Base::as_string();

    const std::string str = date::format("%Y-%m-%d", date);
    return str;
}

bool Fmi::date_time::Date::operator==(const Date& other) const
{
    assert_special();
    return date == other.date;
}

bool Fmi::date_time::Date::operator!=(const Date& other) const
{
    assert_special();
    return date != other.date;
}

bool Fmi::date_time::Date::operator<(const Date& other) const
{
    assert_special();
    return date < other.date;
}

bool Fmi::date_time::Date::operator<=(const Date& other) const
{
    assert_special();
    return date <= other.date;
}

bool Fmi::date_time::Date::operator>(const Date& other) const
{
    assert_special();
    return date > other.date;
}

bool Fmi::date_time::Date::operator>=(const Date& other) const
{
    assert_special();
    return date >= other.date;
}

Fmi::date_time::Date Fmi::date_time::Date::operator+(int num_days) const
{
    assert_special();
    date::local_days tmp(date);
    tmp += date::days(num_days);
    return Date(tmp);
}

Fmi::date_time::Date Fmi::date_time::Date::operator-(int num_days) const
{
    assert_special();
    date::local_days tmp(date);
    tmp -= date::days(num_days);
    return Date(tmp);
}

int Fmi::date_time::Date::operator-(const Date& other) const
{
    assert_special();
    return (date - other.date).count();
}

Fmi::date_time::Date& Fmi::date_time::Date::operator+=(int num_days)
{
    assert_special();
    date += date::days(num_days);
    return *this;
}

Fmi::date_time::Date& Fmi::date_time::Date::operator-=(int num_days)
{
    assert_special();
    date -= date::days(num_days);
    return *this;
}

void Fmi::date_time::Date::assert_special() const
{
    if (is_special())
        throw Fmi::Exception(BCP, "Operation not supported for special values");
}


std::string Fmi::date_time::to_simple_string(const Fmi::date_time::Date& date)
{
    return date.as_string();
}

std::string Fmi::date_time::to_iso_string(const Fmi::date_time::Date& date)
{
    return date.as_iso_string();
}

std::string Fmi::date_time::to_iso_extended_string(const Fmi::date_time::Date& date)
{
    return date.as_iso_extended_string();
}

std::ostream& Fmi::date_time::operator<<(std::ostream& os, const Fmi::date_time::Date& date)
{
    os << date.as_string();
    return os;
}

Fmi::date_time::Date 
Fmi::date_time::Date::from_stream(std::istream& is, bool assume_eoi)
{
    date::year year;
    date::month month;
    date::day day;
    try {
        const internal::StreamExceptionState save(is, std::ios::failbit | std::ios::badbit);
        is >> date::parse("%Y-", year);
        if (!is.eof() && std::isdigit(is.peek())) {
            is >> date::parse("%m-", month);
        } else {
            is.exceptions(std::ios::failbit | std::ios::badbit);
            is >> date::parse("%b-", month);
        }
        is >> date::parse("%d", day);
    } catch (...) {
        is.clear();
        auto err = Fmi::Exception::Trace(BCP, "Failed to parse date from string");
        throw err;
    }

    internal::check_parse_status(is, assume_eoi, "date");

    date::year_month_day ymd(year, month, day);
    date::local_days l_days(ymd);
    return Fmi::date_time::Date(l_days);
}

Fmi::date_time::Date Fmi::date_time::date_from_string(const std::string& str)
{
    std::istringstream is(Fmi::trim_copy(str));
    try
    {
        return Fmi::date_time::Date::from_stream(is, true);
    }
    catch(...)
    {
        auto err = Fmi::Exception::Trace(BCP, "Failed to parse time duration from string '" + str + "'");
        err.addParameter("Error position", internal::handle_parse_remainder(is));
        throw err;
    }
}
