#include "TimeZonePtr.h"

#include "../Exception.h"
#include "../StringConversion.h"

using namespace Fmi::date_time;

TimeZonePtr TimeZonePtr::utc("UTC");

TimeZonePtr::TimeZonePtr() noexcept
    : tz(nullptr)
{
}

TimeZonePtr::TimeZonePtr(const std::string& name)
    : tz(DateTimeNS::locate_zone(name))
{
    if (!tz)
        throw Fmi::Exception(BCP, "Time zone '" + name + "' not found");
}

TimeZonePtr::TimeZonePtr(const DateTimeNS::time_zone* tz) noexcept
    : tz(tz)
{
}

TimeZonePtr::TimeZonePtr(const TimeZonePtr& src) noexcept = default;

TimeZonePtr::~TimeZonePtr() = default;

TimeZonePtr& Fmi::date_time::TimeZonePtr::operator = (const TimeZonePtr& src) noexcept = default;

const Fmi::DateTimeNS::time_zone* Fmi::date_time::TimeZonePtr::zone_ptr() const
{
    if (!tz)
        throw Fmi::Exception(BCP, "Time zone not set");

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
#if 0 && !USE_OS_TZDB
    const auto& links = tzdb.links;
    std::transform(
      links.begin(),
      links.end(),
      std::back_inserter(result),
      [](const DateTimeNS::time_zone_link& tzl) { return tzl.name(); });
#endif
    std::sort(result.begin(), result.end());
    return result;
}





