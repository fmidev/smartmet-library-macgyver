#pragma once

#include <chrono>
#include <boost/date_time/posix_time/posix_time.hpp>

#if __cplusplus < 202002L
#include <date/date.h>
#include <date/tz.h>
#endif

namespace Fmi
{
#if __cplusplus >= 202002L
  namespace date_ns = std::chrono;
#else
  namespace date_ns = date;
#endif

  namespace detail
  {

    using period_t = std::micro;
    using duration_t = std::chrono::duration<int64_t, std::micro>;
    using time_point_t = std::chrono::time_point<date_ns::local_t, duration_t>;
    using seconds_t = std::chrono::duration<int64_t, std::ratio<1, 1> >;
    using days_t = std::chrono::duration<int64_t, std::ratio<86400, 1> >;

    constexpr int periods_per_mks = std::micro::den * period_t::num / period_t::den;

    static_assert (periods_per_mks * period_t::den / period_t::num == std::micro::den,
          "INTERNAL ERROR");
  }

  using Date = boost::gregorian::date;
  using TimeDuration = boost::posix_time::time_duration;

} // namespace Fmi
