#pragma once

#include "DateTimeCommon.h"
#include <iostream>

namespace Fmi
{
  class TimeZone
  {
    const date_ns::time_zone* tz;

  public:
    TimeZone();
      TimeZone(const std::string& name);
    TimeZone(const TimeZone& src);
    virtual ~TimeZone();
    TimeZone& operator = (const TimeZone& src);
    operator const date_ns::time_zone * () const;
  };

}

