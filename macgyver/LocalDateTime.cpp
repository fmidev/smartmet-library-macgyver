#include "LocalDateTime.h"
#include "Exception.h"

Fmi::LocalDateTime::LocalDateTime() = default;

Fmi::LocalDateTime::LocalDateTime(const Fmi::DateTime& time, const Fmi::TimeZone& tz)
    : boost::optional<detail::zoned_time_t>(
        date_ns::zoned_time(
            tz.zone_ptr(),
            time ))
{
}

Fmi::LocalDateTime::LocalDateTime(const Fmi::LocalDateTime&) = default;

Fmi::LocalDateTime::~LocalDateTime() = default;

Fmi::LocalDateTime&
Fmi::LocalDateTime::operator = (const Fmi::LocalDateTime&) = default;

const Fmi::detail::zoned_time_t&
Fmi::LocalDateTime::get_impl() const
{
    if (!*this)
    {
        throw Fmi::Exception(BCP, "Uninitialized Fmi::LocalDateTime");
    }
    return **this;
}

Fmi::TimeZone Fmi::LocalDateTime::zone() const
{
    return Fmi::TimeZone(get_impl().get_time_zone());
}

Fmi::DateTime Fmi::LocalDateTime::local_time() const
{
    return Fmi::DateTime(get_impl().get_local_time());
}

Fmi::DateTime Fmi::LocalDateTime::utc_time() const
{
    const auto& impl = get_impl();
    auto tmp = impl.get_local_time();
    const auto offset = impl.get_info().offset;
    tmp -= offset;
    return Fmi::DateTime(tmp);
}

Fmi::LocalDateTime
Fmi::LocalDateTime::to_tz(const Fmi::TimeZone& zone) const
{
    const auto& impl = get_impl();
    const auto tmp = impl.get_time_zone()->to_sys(impl.get_local_time());
    const auto new_local = zone->to_local(tmp);
    return Fmi::LocalDateTime(new_local, zone);
}
