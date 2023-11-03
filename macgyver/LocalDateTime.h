#pragma once

#include "DateTime.h"
#include "TimeZone.h"
#include <iostream>
#include <boost/optional.hpp>

namespace Fmi
{
  class LocalDateTime : public boost::optional<detail::zoned_time_t>
  {
  public:
    LocalDateTime();
    LocalDateTime(const LocalDateTime& src);
    LocalDateTime(const DateTime& time, const TimeZone& tz);

    virtual ~LocalDateTime();

    LocalDateTime& operator = (const LocalDateTime& src);

    const detail::zoned_time_t& get_impl() const;

    TimeZone zone() const;
    DateTime local_time() const;
    DateTime utc_time() const;
    LocalDateTime to_tz(const TimeZone& zone) const;

  private:
    inline const detail::zoned_time_t& get_impl_nothrow() const { return **this; }
  };
}

