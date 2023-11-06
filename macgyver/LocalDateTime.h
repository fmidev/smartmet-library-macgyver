#pragma once

#include "DateTime.h"
#include <boost/date_time/local_time/local_time.hpp>

namespace Fmi
{
  using LocalDateTime = boost::local_time::local_date_time;
  using TimeZonePtr = boost::local_time::time_zone_ptr;
}
