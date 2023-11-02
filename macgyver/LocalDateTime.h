#pragma once

#include "DateTime.h"
#include <iostream>
#include <boost/optional.hpp>

namespace Fmi
{
  class LocalDateTime : public boost::optional<date_ns::zoned_time>
  {
  public:
    LocalDateTime();
    LocalDateTime(const LocalDateTime& src);
    LocalDateTime(const DateTime& time, const TimeZone& tz);

    virtual ~LocalDateTime();

    LocalDateTime& operator = (const LocalDateTime& src);
  };
}

