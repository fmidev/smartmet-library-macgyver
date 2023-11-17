#include "LocalDateTime.h"
#include "Exception.h"
#include <algorithm>

using namespace Fmi;

TimeZonePtr TimeZonePtr::utc("UTC");

TimeZonePtr::TimeZonePtr() noexcept
    : tz(nullptr)
{
}

TimeZonePtr::TimeZonePtr(const std::string& name)
    : tz(DateTimeNS::locate_zone(name))
{
}

TimeZonePtr::TimeZonePtr(const DateTimeNS::time_zone* tz) noexcept
    : tz(tz)
{
}

TimeZonePtr::TimeZonePtr(const TimeZonePtr& src) noexcept = default;

TimeZonePtr::~TimeZonePtr() = default;

TimeZonePtr& TimeZonePtr::operator = (const TimeZonePtr& src) noexcept = default;

const DateTimeNS::time_zone*
TimeZonePtr::zone_ptr() const
{
    if (!tz)
    {
        throw Exception(BCP, "Uninitialized TimeZonePtr");
    }
    return tz;
}

std::vector<std::string> TimeZonePtr::get_region_list()
{
    std::vector<std::string> result;
    const DateTimeNS::tzdb& regions = DateTimeNS::get_tzdb();
    std::transform(
        regions.zones.begin(),
        regions.zones.end(),
        std::back_inserter(result),
        [](const DateTimeNS::time_zone& tz) { return tz.name(); });
    std::transform(
        regions.links.begin(),
        regions.links.end(),
        std::back_inserter(result),
        [](const DateTimeNS::time_zone_link& tzl) { return tzl.name(); });
    std::sort(result.begin(), result.end());
    return result;
}


LocalDateTime::LocalDateTime()
    : boost::optional<detail::zoned_time_t>()
    , type(detail::TimeType::NOT_A_DATE_TIME)
{
}

LocalDateTime::LocalDateTime(Type type)
    : boost::optional<detail::zoned_time_t>()
    , type(type)
{
    // NORMAL cannot be used in this context
    if (type == Type::NORMAL)
    {
        throw Exception(BCP, "Invalid LocalDateTime type");
    }
}

LocalDateTime::LocalDateTime(boost::posix_time::special_values sv)
    : boost::optional<detail::zoned_time_t>()
    , type(detail::get_time_type(sv))
{
}

//LocalDateTime::LocalDateTime(const boost::posix_time::ptime& time, const TimeZonePtr& tz)
//    : boost::optional<detail::zoned_time_t>(
//        detail::zoned_time_t(
//            tz.zone_ptr(),
//            detail::ptime_to_time_point(time) ))
//{
//}

LocalDateTime::LocalDateTime(const DateTime& time, const TimeZonePtr& tz)
    : boost::optional<detail::zoned_time_t>(
        [&time, &tz]() -> boost::optional<detail::zoned_time_t>
        {
            using namespace DateTimeNS;
            using namespace detail;
            if (time.is_special())
                return boost::none;

            const time_point_t tpoint = ptime_to_time_point(time);
            try
            {            
                return boost::optional<zoned_time_t>(zoned_time_t(tz.zone_ptr(), tpoint));
            }
            catch (const nonexistent_local_time& e)
            {
                return boost::none;
            }
            catch (const ambiguous_local_time& e)
            {
                const detail::zoned_time_t tmp(tz.zone_ptr(), tpoint, choose::latest);
                if (tmp.get_local_time() == tpoint)
                    return tmp;
                return boost::none;
            }
            catch (const std::exception& e)
            {
                throw Exception::Trace(BCP, "Failed to construct LocalDateTime")
                    .addParameter("time", boost::posix_time::to_iso_extended_string(time))
                    .addParameter("tz", tz.name())
                    .addParameter("what", e.what());
            }
        } () )
{
    const bool has_time = boost::optional<detail::zoned_time_t>::operator bool();
    if (time.is_special())
    {
        type = detail::get_time_type(time);
    }
    else if (!has_time)
    {
        type = detail::TimeType::NOT_A_DATE_TIME;
    }
    else
    {
        type = detail::TimeType::NORMAL;
    }
}

LocalDateTime::LocalDateTime(const Date& date, const TimeDuration& time, const TimeZonePtr& tz,
      boost::local_time::local_date_time::DST_CALC_OPTIONS opt)
try
    : LocalDateTime(DateTime(date, time), tz)
{
}
catch(const std::exception& e)
{
    switch (opt)
    {
        case boost::local_time::local_date_time::NOT_DATE_TIME_ON_ERROR:
            *this = LocalDateTime(Type::NOT_A_DATE_TIME);
            break;
        default:
            throw;
    }
}

LocalDateTime::LocalDateTime(const detail::time_point_t& time, const TimeZonePtr& tz)
    : boost::optional<detail::zoned_time_t>(
        detail::zoned_time_t(
            tz.zone_ptr(),
            time))
    , type(detail::TimeType::NORMAL)
{
}

LocalDateTime::LocalDateTime(const LocalDateTime&) = default;

LocalDateTime::~LocalDateTime() = default;

LocalDateTime& LocalDateTime::operator = (const LocalDateTime&) = default;

const detail::zoned_time_t& LocalDateTime::get_impl() const
{
    if (!*this)
    {
        throw Exception(BCP, "Uninitialized LocalDateTime");
    }
    return **this;
}

TimeZonePtr LocalDateTime::zone() const
{
    return TimeZonePtr(get_impl().get_time_zone());
}

DateTime LocalDateTime::local_time() const
{
    detail::time_point_t tmp = get_impl().get_local_time();
    return DateTime(detail::time_point_to_ptime(tmp));
}

DateTime LocalDateTime::utc_time() const
{
    return to_tz(TimeZonePtr::utc).local_time();
}

LocalDateTime LocalDateTime::to_tz(const TimeZonePtr& zone) const
{
    if (is_special())
    {
        return *this;
    }
    const auto& impl = get_impl();
    const auto tmp = impl.get_time_zone()->to_sys(impl.get_local_time());
    const auto new_local = zone.zone_ptr()->to_local(tmp);
    return LocalDateTime(new_local, zone);
}

bool LocalDateTime::dst_on() const
{
    return get_impl().get_info().save != std::chrono::seconds(0);
}

double LocalDateTime::base_offset() const
{
    const auto offset = std::chrono::duration_cast<std::chrono::minutes>(
        get_impl().get_info().offset);
    return offset.count() / 60.0;
}

double LocalDateTime::dst_offset() const
{
    const auto offset = std::chrono::duration_cast<std::chrono::minutes>(
        get_impl().get_info().save);
    return get_impl().get_info().save.count() / 60.0;
}

void LocalDateTime::printOn(std::ostream& os) const
{
    if (is_special())
    {
        switch (type)
        {
            case Type::NORMAL:
                os << "NORMAL";
                break;
            case Type::NOT_A_DATE_TIME:
                os << "NOT_A_DATE_TIME";
                break;
            case Type::NEG_INFINITY:
                os << "NEG_INFINITY";
                break;
            case Type::POS_INFINITY:
                os << "POS_INFINITY";
                break;
        }
    }
    else
    {
        std::cout << boost::posix_time::to_iso_extended_string(local_time());
        std::cout << " " << **this;
    }
}

bool LocalDateTime::operator < (const LocalDateTime& other) const
{
    if (is_not_a_date_time() || other.is_not_a_date_time())
    {
        return false;
    }
    else if (is_special() || other.is_special())
    {
        if (type == other.type)
            return false;

        return type < other.type;
    }

    if (get_impl().get_time_zone() == other.get_impl().get_time_zone())
    {
        return get_impl().get_local_time() < other.get_impl().get_local_time();
    }

    return get_impl().get_local_time() < other.to_tz(get_impl().get_time_zone()).get_impl().get_local_time();
}

bool LocalDateTime::operator == (const LocalDateTime& other) const
{
    if (is_special() || other.is_special())
    {
        return type == other.type;
    }

    if (get_impl().get_time_zone() == other.get_impl().get_time_zone())
    {
        return get_impl().get_local_time() == other.get_impl().get_local_time();
    }

    return get_impl().get_local_time() == other.to_tz(get_impl().get_time_zone()).get_impl().get_local_time();    
}

bool LocalDateTime::operator > (const LocalDateTime& other) const
{
    if (is_not_a_date_time() || other.is_not_a_date_time())
    {
        return false;
    }
    else if (is_special() || other.is_special())
    {
        if (type == other.type)
            return false;

        return type > other.type;
    }

    if (get_impl().get_time_zone() == other.get_impl().get_time_zone())
    {
        return get_impl().get_local_time() > other.get_impl().get_local_time();
    }

    return get_impl().get_local_time() > other.to_tz(get_impl().get_time_zone()).get_impl().get_local_time();
}

std::string Fmi::to_iso_string(const LocalDateTime& time)
{
    if (time.is_special())
        return "";
    
    const detail::time_point_t tpoint = time->get_local_time();
    const TimeZonePtr tz = time.zone();

    const auto info = time->get_info();
    const auto offset = info.offset + info.save;
    
    std::ostringstream os;
    os << detail::to_iso_string(tpoint);
    if (offset.count() == 0)
    {
        os << 'Z';
    }
    else
    {
        os << DateTimeNS::format("%z", offset);
    }
    return os.str();
}

std::string Fmi::to_iso_extended_string(const LocalDateTime& time)
{
    using namespace std::chrono;
    using namespace detail;
    using namespace DateTimeNS;

    const time_point_t tpoint = time->get_local_time();
    const TimeZonePtr tz = time.zone();
    
    const auto info = time->get_info();
    const int offset_min = duration_cast<minutes>(info.offset + info.save).count();

    std::ostringstream os;
    os << Fmi::detail::to_iso_extended_string(tpoint);
    if (offset_min == 0)
    {
        os << 'Z';
    }
    else
    {
        os << (offset_min > 0 ? '+' : '-');
        const int hours = std::abs(offset_min) / 60;
        const int minutes = std::abs(offset_min) % 60;
        os << std::setw(2) << std::setfill('0') << hours;
        os << ':';
        os << std::setw(2) << std::setfill('0') << minutes;
    }
    return os.str();
}

void LocalDateTime::advance(const TimeDuration& td)
{
    if (is_special()) {
        return;
    }

    if (td.is_special())
    {
        type = detail::TimeType::NOT_A_DATE_TIME;
        boost::optional<detail::zoned_time_t>::operator=(boost::none);
        return;
    }

    const auto mks = td.total_microseconds();
    const detail::duration_t adv =
        std::chrono::duration_cast<detail::duration_t>(
            std::chrono::microseconds(mks));
    const DateTimeNS::time_zone* tz = get_impl().get_time_zone();
    const auto& impl = get_impl();
    const auto sys_pt = tz->to_sys(impl.get_local_time());
    const auto new_sys_pt = sys_pt + adv;
    const auto new_local_pt = tz->to_local(new_sys_pt);
    boost::optional<detail::zoned_time_t>::operator=(
        detail::zoned_time_t(tz, new_local_pt));
}

LocalDateTime Fmi::operator + (const LocalDateTime& time, const TimeDuration& td)
{
    LocalDateTime tmp(time);
    tmp.advance(td);
    return tmp;
}

LocalDateTime Fmi::operator - (const LocalDateTime& time, const TimeDuration& td)
{
    LocalDateTime tmp(time);
    tmp.advance(-td);
    return tmp;
}

TimeDuration Fmi::operator - (const LocalDateTime& from, const LocalDateTime& to)
{
    if (from.is_special() || to.is_special())
    {
        return TimeDuration();
    }

    const auto* tz_from = from.get_impl().get_time_zone();
    const auto* tz_to = to.get_impl().get_time_zone();

    const auto diff = tz_to->to_sys(to.get_impl().get_local_time()) -
        tz_from->to_sys(from.get_impl().get_local_time());

    const auto mks = std::chrono::duration_cast<std::chrono::microseconds>(diff).count();
    return Fmi::Microseconds(mks);
}
