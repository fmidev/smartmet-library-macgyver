#include "TimeZone.h"
#include "Exception.h"

Fmi::TimeZone::TimeZone() noexcept
    : tz(nullptr)
{
}

Fmi::TimeZone::TimeZone(const std::string& name)
try
    : tz(date_ns::locate_zone(name))
{
}
catch (...)
{
    throw Fmi::Exception::Trace(BCP, "Failed to locate time zone '" + name + "'");
}

Fmi::TimeZone::TimeZone(const date_ns::time_zone* tz) noexcept
    : tz(tz)
{
}

Fmi::TimeZone::TimeZone(const TimeZone& src) noexcept = default;
Fmi::TimeZone::~TimeZone() = default;
Fmi::TimeZone& Fmi::TimeZone::operator = (const Fmi::TimeZone& src) noexcept = default;

const Fmi::date_ns::time_zone*
Fmi::TimeZone::zone_ptr() const
{
    if (!tz)
    {
        throw Fmi::Exception(BCP, "Uninitialized Fmi::TimeZone");
    }
    return tz;
}
