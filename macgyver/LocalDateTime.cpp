#include "LocalDateTime.h"

Fmi::LocalDateTime::LocalDateTime() = default;

Fmi::LocalDateTime::LocalDateTime(const DateTime& time, const TimeZone& tz)
    : boost::optional<date_ns::zoned_time>(
        date_ns::zoned_time(time, tz))
{
}

Fmi::LocalDateTime::LocalDateTime(const Fmi::LocalDateTime&) = default;

Fmi::LocalDateTime::~LocalDateTime() = default;

Fmi::LocalDateTime&
Fmi::LocalDateTime::operator = (const Fmi::LocalDateTime&) = default;
