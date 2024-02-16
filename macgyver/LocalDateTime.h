#pragma once

#include "DateTime.h"

#ifdef USE_BOOST_DATE_TIME

#include <boost/date_time/local_time/local_time.hpp>

namespace Fmi
{
  using LocalDateTime = boost::local_time::local_date_time;
  using TimeZonePtr = boost::local_time::time_zone_ptr;
}

#else

#include "date_time/LocalDateTime.h"

namespace Fmi
{
  using LocalDateTime = date_time::LocalDateTime;
  using LocalTimePeriod = date_time::TimePeriod<date_time::LocalDateTime>;

  using TimeZonePtr = date_time::TimeZonePtr;

  using date_time::make_time;
}

#endif
