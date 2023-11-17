#pragma once

#include <chrono>
#if __cplusplus >= 202002L && defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L
// Time zone, zoned_time etc is supported in libstdc++ => use it
namespace Fmi
{
  namespace DateTimeNS = std::chrono;
}
#else
// Time zone, zoned_time etc is NOT supported in libstdc++ or support is incomplete
//      => use date library
#include <date/date.h>
#include <date/tz.h>
namespace Fmi
{
  namespace DateTimeNS = date;
}
#endif

#include <boost/date_time/posix_time/posix_time.hpp>

namespace Fmi
{
  using Date = boost::gregorian::date;
  using TimeDuration = boost::posix_time::time_duration;
  using DateTime = boost::posix_time::ptime;

  using Seconds = boost::posix_time::seconds;
  using Minutes = boost::posix_time::minutes;
  using Hours = boost::posix_time::hours;
  using Milliseconds = boost::posix_time::milliseconds;
  using Microseconds = boost::posix_time::microseconds;
  using MicrosecClock = boost::posix_time::microsec_clock;
  using SecondClock = boost::posix_time::second_clock;

  namespace detail
  {

    using period_t = std::micro;
    using duration_t = std::chrono::duration<int64_t, std::micro>;
    using time_point_t = std::chrono::time_point<DateTimeNS::local_t, duration_t>;
    using zoned_time_t = DateTimeNS::zoned_time<duration_t>;
    using seconds_t = std::chrono::duration<int64_t, std::ratio<1, 1> >;
    using minutes_t = std::chrono::duration<int64_t, std::ratio<60, 1> >;
    using hours_t = std::chrono::duration<int64_t, std::ratio<3600, 1> >;
    using days_t = std::chrono::duration<int64_t, std::ratio<86400, 1> >;
    using day_t = DateTimeNS::day;
    using month_t = DateTimeNS::month;
    using year_t = DateTimeNS::year;
    using ymd_t = DateTimeNS::year_month_day;

    constexpr int periods_per_mks = std::micro::den * period_t::num / period_t::den;

    extern const time_point_t epoch_time_point;
    extern const boost::posix_time::ptime epoch_ptime;

    static_assert (periods_per_mks * period_t::den / period_t::num == std::micro::den,
          "INTERNAL ERROR");

    time_point_t ptime_to_time_point(const DateTime& ptime);
  
    boost::posix_time::ptime time_point_to_ptime(const time_point_t& time_point);

    enum class TimeType
    {
      NORMAL = 0,
      NOT_A_DATE_TIME = -2,
      NEG_INFINITY = -1,
      POS_INFINITY = 1
    };

    TimeType get_time_type(const DateTime& time);
    TimeType get_time_type(boost::posix_time::special_values sv);

    std::string to_iso_string(const time_point_t& time);
    std::string to_iso_extended_string(const time_point_t& time);
  }

  // FIXME: to be handled later
  //     boost::posix_time::ptime_from_tm
  //     boost::posix_time::from_time_t
  //     boost::date_time::neg_infin
  //     boost::date_time::pos_infin
}
