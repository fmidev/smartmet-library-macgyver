#pragma once

#include "DateTimeCommon.h"
#include <ostream>

namespace Fmi
{

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
    DateTime(const detail::time_point_t& tp);
    DateTime(const DateTime& src);

    virtual ~DateTime();

    DateTime& operator = (const DateTime& src);
    DateTime& operator = (const boost::posix_time::ptime& src);
    Date date() const;
    TimeDuration time_of_day() const;

    operator boost::posix_time::ptime () const;

    std::string to_simple_string() const;
    std::string to_iso_string() const;
    std::string to_iso_extended_string() const;

    inline bool is_special() const { return !initialized; }
    inline bool is_not_a_date_time() const { return !initialized; }
  };

  std::ostream& operator << (std::ostream& os, const DateTime& src);

  std::string to_simple_string(const DateTime& src);
  std::string to_iso_string(const DateTime& src);
  std::string to_iso_extended_string(const DateTime& src);
}
