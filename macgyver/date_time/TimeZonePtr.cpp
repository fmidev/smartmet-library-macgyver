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

#include <iostream>

std::vector<std::string> TimeZonePtr::get_region_list()
{
    // It seems that Date library (date/tz.cpp) returns additional
    // wrong names beginning with lowercase letter when using the system TZDB.
    // Require that all names must begin with an uppercase letter.
    std::vector<std::string> result;
    const Fmi::DateTimeNS::tzdb& regions = Fmi::DateTimeNS::get_tzdb();
    for (const auto& r : regions.zones)
    {
        const std::string& name = r.name();
        if (!name.empty() && std::isupper(*name.begin()))
            result.push_back(name);
    }

#if (defined(USE_OS_TZDB) && USE_OS_TZDB==0) || defined(FMI_CALENDAR_USES_STD_CHRONO)
    for (const auto& link : regions.links)
    {
        const std::string& name = link.name();
        if (!name.empty() && std::isupper(*name.begin()))
            result.push_back(name);
    }
#endif
    std::sort(result.begin(), result.end());
    return result;
}

std::map<std::string, TimeZonePtr> TimeZonePtr::get_region_map(bool debug)
{
    std::map<std::string, TimeZonePtr> result;
    const Fmi::DateTimeNS::tzdb& regions = Fmi::DateTimeNS::get_tzdb();
    for (const auto& r : regions.zones)
    {
        const std::string& name = r.name();
        if (!name.empty() && std::isupper(*name.begin()))
        {
            result[name] = &r;
            if (debug)
                std::cout << "TIME ZONE: " << name << std::endl;
        }
    }

#if (defined(USE_OS_TZDB) && USE_OS_TZDB==0) || defined(FMI_CALENDAR_USES_STD_CHRONO)
    for (const auto& link : regions.links)
    {
        const std::string& name = link.name();
        if (!name.empty() && std::isupper(*name.begin()))
        {
            try
            {
                const std::string& target = link.target();
                const TimeZonePtr tz(result.at(target));
                result[name] = tz;
                if (debug)
                    std::cout << "TIME ZONE LINK: " << name << " -> " << target << std::endl;
            }
            catch(const std::exception& e)
            {
                Fmi::Exception err(BCP, "Failed to resolve time zone link");
                err.addParameter("name", name);
                err.addParameter("target", link.target());
                throw err;
            }
        }
    }
#endif
    return result;
}
