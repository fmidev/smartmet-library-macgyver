#pragma once

#include "DateTimeCommon.h"
#include <iostream>

namespace Fmi
{
  class TimeZone
  {
    const date_ns::time_zone* tz;

  public:
    TimeZone() noexcept;
    TimeZone(const std::string& name);
    TimeZone(const date_ns::time_zone* tz) noexcept;
    TimeZone(const TimeZone& src) noexcept;
    virtual ~TimeZone();
    TimeZone& operator = (const TimeZone& src) noexcept;
    inline operator bool () const noexcept { return tz != nullptr; }
    inline operator const date_ns::time_zone * () const { return zone_ptr(); }
    inline const date_ns::time_zone * operator -> () const { return zone_ptr(); }
    const date_ns::time_zone* zone_ptr() const;
  };
}
