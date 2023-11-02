#include "TimeZone.h"
#include "Exception.h"

namespace
{
    template <typename Type>
    const Type* check_ptr(const Type* ptr)
    {
        if (ptr)
            return ptr;

        throw Fmi::Exception(BCP, "NULL pointer encountered");
    }
}



Fmi::TimeZone::TimeZone()
    : tz(nullptr)
{
}

Fmi::TimeZone::TimeZone(const TimeZone& src)= default;
Fmi::TimeZone::~TimeZone() = default;
Fmi::TimeZone& Fmi::TimeZone::operator = (const Fmi::TimeZone& src) = default;

Fmi::TimeZone::operator const date_ns::time_zone * () const
try
{
    return check_ptr(tz);
}
catch (...)
{
    throw Fmi::Exception::Trace(BCP, "Uninitialized time zone object");
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

