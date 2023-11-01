#pragma once

#include <chrono>
#include <ostream>
#include <boost/date_time/posix_time/posix_time.hpp>

#if __cplusplus < 202002L
#include <date/date.h>
#endif

namespace Fmi
{
#if __cplusplus >= 202002L
  namespace date_ns = std::chrono;
#else
  namespace date_ns = date;
#endif

  using Date = boost::gregorian::date;
  using TimeDuration = boost::posix_time::time_duration;

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

  class DateTime : public detail::time_point_t
  {
    using base = detail::time_point_t;
    /**
     *  @brief Specifies whether object is initialized
     *
     *  We cannot unfortunately use comparison with uninitialized std::chrono::time_point
     *  as it would exclude Jan 1st, 1970 klo 00:00:00 from use
     */
    bool initialized;
  public:
    DateTime();
    DateTime(const boost::posix_time::ptime& src);
    DateTime(const Date& date, const TimeDuration& t_diff);
    DateTime(const DateTime& src);

    virtual ~DateTime();

    DateTime& operator = (const boost::posix_time::ptime& src);
    Date date() const;
    TimeDuration time_of_day() const;

    operator boost::posix_time::ptime () const;

    std::string to_simple_string() const;
    std::string to_iso_string() const;
    std::string to_iso_extended_string() const;
  };

  std::ostream& operator << (std::ostream& os, const DateTime& src);

  std::string to_simple_string(const DateTime& src);
  std::string to_iso_string(const DateTime& src);
  std::string to_iso_extended_string(const DateTime& src);
}
