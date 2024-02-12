#include "LocalDateTime.h"
#include "TimeZonePtr.h"
#include "Internal.hpp"
#include "../Exception.h"

using Fmi::date_time::Date;
using Fmi::date_time::DateTime;
using Fmi::date_time::TimeDuration;
using Fmi::date_time::TimeZonePtr;
using Fmi::date_time::LocalDateTime;

namespace detail = Fmi::detail;

LocalDateTime::LocalDateTime()
    : Base()
{
}

LocalDateTime::LocalDateTime(Type type)
    : Base(type)
{
}

LocalDateTime::LocalDateTime(const LocalDateTime& src) = default;

LocalDateTime::LocalDateTime(const detail::zoned_time_t& zoned_time)
    : Base(NORMAL)
    , ldt(zoned_time)
{
}

LocalDateTime::LocalDateTime(
    const DateTime& time,
    const TimeZonePtr& tz,
    enum ErrorHandling err_handling)

    : Base(NORMAL)
{
    if (!tz)
        throw Fmi::Exception(BCP, "Time zone not set");

    try
    {
        ldt = detail::zoned_time_t(
            tz.zone_ptr(),
            detail::time_point_t(time.get_impl()) );

        set_type(NORMAL);
    }
    catch(const std::exception& e)
    {
        if (err_handling == EXCEPTION_ON_ERROR)
            throw Fmi::Exception::Trace(BCP, e.what());
            
        set_type(NOT_A_DATE_TIME);
    }
    
}

LocalDateTime::LocalDateTime(
    const Date& date,
    const TimeDuration& time,
    const TimeZonePtr& tz,
    enum ErrorHandling err_handling)

    : Base()
{
    if (!tz)
        throw Fmi::Exception(BCP, "Time zone not set");

    try
    {
        ldt = detail::zoned_time_t(
            tz.zone_ptr(),
            detail::time_point_t(date.get_impl() + time.get_impl()) );
        set_type(NORMAL);
    }
    catch(...)
    {
        if (err_handling == EXCEPTION_ON_ERROR)
            throw Fmi::Exception::Trace(BCP, "Operation failed!");

        set_type(NOT_A_DATE_TIME);
    }
}

LocalDateTime::LocalDateTime(
    const detail::time_point_t& time,
    const DateTimeNS::time_zone* tz,
    enum ErrorHandling err_handling)

    : Base(NORMAL)
{
    if (!tz)
        throw Fmi::Exception(BCP, "Time zone not set");

    try
    {
        ldt = detail::zoned_time_t(tz, time);
        set_type(NORMAL);
    }
    catch (...)
    {
        if (err_handling == EXCEPTION_ON_ERROR)
            throw Fmi::Exception::Trace(BCP, "Operation failed!");

        set_type(NOT_A_DATE_TIME);
    }
}

LocalDateTime::~LocalDateTime() = default;

LocalDateTime& LocalDateTime::operator = (const LocalDateTime& src) = default;

const Fmi::detail::zoned_time_t& LocalDateTime::get_impl() const
{
    check_no_special(BCP);
    return ldt;
}

void LocalDateTime::check_no_special(
    const char* _filename,
    int _line,
    const char* _function) const
{
    if (is_special())
        throw Fmi::Exception(_filename, _line, _function, "Not supported for special LocalDateTime values");
}

Date LocalDateTime::date() const
{
    check_no_special(BCP);
    return local_time().date();
}

DateTime LocalDateTime::local_time() const
{
    check_no_special(BCP);
    return DateTime(ldt.get_local_time());
}

TimeZonePtr LocalDateTime::zone() const
{
    check_no_special(BCP);
    return TimeZonePtr(ldt.get_time_zone());
}

TimeDuration LocalDateTime::time_of_day() const
{
    return local_time().time_of_day();
}

DateTime LocalDateTime::utc_time() const
{
    TimeDuration diff(get_sys_info().offset);
    return local_time() - diff;
}

LocalDateTime LocalDateTime::to_tz(const TimeZonePtr& zone) const
{
    check_no_special(BCP);
    const auto& impl = get_impl();
    const auto tmp = impl.get_time_zone()->to_sys(impl.get_local_time());
    const auto new_local = zone.zone_ptr()->to_local(tmp);
    return LocalDateTime(new_local, zone.zone_ptr());
}

bool LocalDateTime::dst_on() const
{
    check_no_special(BCP);
    return get_sys_info().save != std::chrono::seconds(0);
}

double LocalDateTime::offset() const
{
    check_no_special(BCP);
    const auto sys_info = get_sys_info();
    const detail::seconds_t off = sys_info.offset;
    return off.count() / 3600.0;
}

#if !USE_OS_TZDB
double LocalDateTime::dst_offset() const
{
    check_no_special(BCP);
    const auto sys_info = get_sys_info();
    const detail::seconds_t save = sys_info.save;
    return save.count() / 3600;
}
#endif

std::string LocalDateTime::abbrev() const
{
    check_no_special(BCP);
    return get_sys_info().abbrev;
}

Fmi::DateTimeNS::sys_info LocalDateTime::get_sys_info() const
{
    check_no_special(BCP);
    return ldt.get_info();
}

std::pair<detail::sys_time_t, detail::sys_time_t>
LocalDateTime::get_dst_times() const
{
    std::pair<detail::sys_time_t, detail::sys_time_t> result;
    auto info = get_sys_info();
    if (info.save == std::chrono::seconds(0))
    {
        const auto next = info.end + std::chrono::seconds(2);
        info = get_impl().get_time_zone()->get_info(next);
        result = std::make_pair(info.begin, info.end);
    }
    else
    {
        result = std::make_pair(info.begin, info.end);
    }
    return result;
}

std::string LocalDateTime::as_simple_string() const
{
    switch (type())
    {
        case Type::NORMAL:
            return internal::remove_trailing_zeros(local_time().as_string())
                    + " " + abbrev();

        case Type::NOT_A_DATE_TIME:
            return "not-a-date-time";

        case Type::NEG_INFINITY:
            return "NEG_INFINITY";

        case Type::POS_INFINITY:
            return "POS_INFINITY";

        default:
            throw Fmi::Exception(BCP, "INTERNAL ERROR: Invalid LocalDateTime type");
    }
}

std::string LocalDateTime::as_iso_string() const
{
    check_no_special(BCP);
    const std::string s_dt = local_time().as_iso_string();
    const std::string s_zone = zone().zone_ptr() == TimeZonePtr::utc.zone_ptr()
        ? "Z"
        : Fmi::DateTimeNS::format("%z", ldt);
    return s_dt + s_zone;        
}

std::string LocalDateTime::as_iso_extended_string() const
{
    check_no_special(BCP);
    const std::string s_dt = local_time().as_iso_extended_string();
    const std::string s_zone = zone().zone_ptr() == TimeZonePtr::utc.zone_ptr()
        ? "Z"
        : Fmi::DateTimeNS::format("%Ez", ldt);
    return s_dt + s_zone;        
}

void LocalDateTime::advance(const TimeDuration& td)
{
    if (is_special()) {
        return;
    }

    if (td.is_special())
    {
        set_type(NOT_A_DATE_TIME);
        return;
    }

    const auto mks = td.total_microseconds();
    const detail::duration_t adv =
        std::chrono::duration_cast<detail::duration_t>(
            std::chrono::microseconds(mks));
    const DateTimeNS::time_zone* tz = get_impl().get_time_zone();
    const auto& impl = get_impl();
    const auto sys_pt = impl.get_sys_time();
    const auto new_sys_pt = sys_pt + adv;
    const detail::zoned_time_t new_local_pt(tz, new_sys_pt);
    ldt = detail::zoned_time_t(tz, new_local_pt);
}

bool LocalDateTime::operator == (const LocalDateTime& other) const
{
    return compare_with(other) == 0;
}

bool LocalDateTime::operator != (const LocalDateTime& other) const
{
    return compare_with(other) != 0;
}

bool LocalDateTime::operator < (const LocalDateTime& other) const
{
    return compare_with(other) < 0;
}

bool LocalDateTime::operator > (const LocalDateTime& other) const
{
    return compare_with(other) > 0;
}

bool LocalDateTime::operator <= (const LocalDateTime& other) const
{
    return compare_with(other) <= 0;
}

bool LocalDateTime::operator >= (const LocalDateTime& other) const
{
    return compare_with(other) >= 0;
}

int LocalDateTime::compare_with(const LocalDateTime& other) const
{
    if (is_special() || other.is_special())
    {
        check_no_special(BCP);
        other.check_no_special(BCP);

        if (type() == other.type())
            return 0;

        return type() < other.type() ? -1 : 1;
    }

    if (get_impl().get_time_zone() == other.get_impl().get_time_zone())
    {
        if (get_impl().get_local_time() == other.get_impl().get_local_time())
            return 0;

        return get_impl().get_local_time() < other.get_impl().get_local_time() ? -1 : 1;
    }

    const auto other_local = other.to_tz(get_impl().get_time_zone()).get_impl().get_local_time();
    if (get_impl().get_local_time() == other_local)
        return 0;

    return get_impl().get_local_time() < other_local ? -1 : 1;
}

LocalDateTime Fmi::date_time::operator + (const LocalDateTime& time, const TimeDuration& td)
{
    LocalDateTime tmp(time);
    tmp.advance(td);
    return tmp;
}

LocalDateTime Fmi::date_time::operator - (const LocalDateTime& time, const TimeDuration& td)
{
    LocalDateTime tmp(time);
    tmp.advance(-td);
    return tmp;
}

TimeDuration Fmi::date_time::operator - (const LocalDateTime& to, const LocalDateTime& from)
{
    if (from.is_special() || to.is_special())
    {
        return TimeDuration();
    }

    const auto sys_from = from.get_impl().get_sys_time();
    const auto sys_to = to.get_impl().get_sys_time();
    return Fmi::date_time::TimeDuration(sys_to - sys_from);
}

std::string Fmi::date_time::to_simple_string(const LocalDateTime& time)
{
    return time.as_simple_string();
}

std::string Fmi::date_time::to_iso_string(const LocalDateTime& time)
{
    return time.as_iso_string();
}

std::string Fmi::date_time::to_iso_extended_string(const LocalDateTime& time)
{
    return time.as_iso_extended_string();
}

std::ostream& Fmi::date_time::operator << (std::ostream& os, const LocalDateTime& time)
{
    os << time.as_simple_string();
    return os;
}

Fmi::date_time::DateTime Fmi::date_time::MicrosecClock::universal_time()
{
    const auto tmp =
        Fmi::DateTimeNS::clock_time_conversion<Fmi::DateTimeNS::local_t, DateTimeNS::utc_clock>()
            (DateTimeNS::utc_clock::now());
    return std::chrono::time_point_cast<Fmi::detail::microsec_t>(tmp);
}

Fmi::date_time::DateTime Fmi::date_time::MicrosecClock::local_time()
{
    namespace date = Fmi::DateTimeNS;
    TimeZonePtr tz(DateTimeNS::current_zone());
    const LocalDateTime utc_time(Fmi::date_time::MicrosecClock::universal_time(), TimeZonePtr::utc);
    return utc_time.to_tz(tz).local_time();
}

Fmi::date_time::DateTime Fmi::date_time::SecondClock::universal_time()
{
    const auto tmp =
        Fmi::DateTimeNS::clock_time_conversion<Fmi::DateTimeNS::local_t, DateTimeNS::utc_clock>()
            (DateTimeNS::utc_clock::now());
    return std::chrono::time_point_cast<Fmi::detail::microsec_t>(
        std::chrono::time_point_cast<Fmi::detail::seconds_t>(tmp));
}

Fmi::date_time::DateTime Fmi::date_time::SecondClock::local_time()
{
    namespace date = Fmi::DateTimeNS;
    TimeZonePtr tz(DateTimeNS::current_zone());
    const LocalDateTime utc_time(Fmi::date_time::SecondClock::universal_time(), TimeZonePtr::utc);
    return utc_time.to_tz(tz).local_time();
    namespace date = Fmi::DateTimeNS;
}

#include <iostream>

Fmi::date_time::LocalDateTime
Fmi::date_time::make_time(
    const Fmi::date_time::Date& date,
    const Fmi::date_time::TimeDuration& time,
    const Fmi::date_time::TimeZonePtr& tz)
{
    try
    {
        detail::time_point_t tp = date.get_impl() + time.get_impl();
        try
        {
            detail::zoned_time_t ldt(tz.zone_ptr(), tp);
            return LocalDateTime(ldt);
        }
        catch (const DateTimeNS::ambiguous_local_time&)
        {
            // We have ambigous local time - preffer sommertime
            detail::zoned_time_t ldt(tz.zone_ptr(), tp, DateTimeNS::choose::earliest);
            return LocalDateTime(ldt);
        }
        catch (const DateTimeNS::nonexistent_local_time&)
        {
            // We have non-existent local time - preffer standard time
            detail::zoned_time_t ldt(tz.zone_ptr(), tp, DateTimeNS::choose::earliest);
            return LocalDateTime(ldt);
        }
    }
    catch (...)
    {
        return LocalDateTime(LocalDateTime::NOT_A_DATE_TIME);
    }
}
