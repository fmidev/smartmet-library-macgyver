#pragma once

#include <chrono>
#include "DateTime.h"
#include <string>
#include <vector>
#include <boost/optional.hpp>
#include <boost/date_time/local_time/local_time.hpp>

namespace Fmi
{
  class TimeZonePtr
  {
    const DateTimeNS::time_zone* tz;

  public:
    TimeZonePtr() noexcept;
    TimeZonePtr(const std::string& name);
    TimeZonePtr(const DateTimeNS::time_zone* tz) noexcept;
    TimeZonePtr(const TimeZonePtr& src) noexcept;
    virtual ~TimeZonePtr();
    TimeZonePtr& operator = (const TimeZonePtr& src) noexcept;
    inline operator bool () const noexcept { return bool(tz); }
    inline operator const DateTimeNS::time_zone * () const { return zone_ptr(); }
    inline const DateTimeNS::time_zone * operator -> () const { return zone_ptr(); }
    const DateTimeNS::time_zone* zone_ptr() const;

    std::string name() const { return tz->name(); }

    static TimeZonePtr utc;

    static std::vector<std::string> get_region_list();
  };

  class LocalDateTime : public boost::optional<detail::zoned_time_t>
  {
  public:
    using Type = detail::TimeType;

    using Special = detail::TimeType;
    static constexpr auto EXCEPTION_ON_ERROR = boost::local_time::local_date_time::EXCEPTION_ON_ERROR;
    static constexpr auto NOT_DATE_TIME_ON_ERROR = boost::local_time::local_date_time::NOT_DATE_TIME_ON_ERROR;

    LocalDateTime();
    explicit LocalDateTime(Type type);
    explicit LocalDateTime(boost::posix_time::special_values sv);
    LocalDateTime(const LocalDateTime& src);
    LocalDateTime(const boost::posix_time::ptime& time, const TimeZonePtr& tz);
    LocalDateTime(const Date& date, const TimeDuration& time, const TimeZonePtr& tz,
      boost::local_time::local_date_time::DST_CALC_OPTIONS =
        NOT_DATE_TIME_ON_ERROR);

    //LocalDateTime(const DateTime& time, const TimeZonePtr& tz);
    LocalDateTime(const detail::time_point_t& time, const TimeZonePtr& tz);

    virtual ~LocalDateTime();

    LocalDateTime& operator = (const LocalDateTime& src);

    const Fmi::detail::zoned_time_t& get_impl() const;

    TimeZonePtr zone() const;
    DateTime local_time() const;
    Date date() const { return local_time().date(); }

    DateTime utc_time() const;
    LocalDateTime to_tz(const TimeZonePtr& zone) const;

    bool dst_on() const;
    double base_offset() const;
    double dst_offset() const;

    void printOn(std::ostream& os) const;

    bool is_not_a_date_time() const { return type == detail::TimeType::NOT_A_DATE_TIME; };
    bool is_infinity() const { return type == detail::TimeType::POS_INFINITY || type == detail::TimeType::NEG_INFINITY; };
    bool is_neg_infinity() const { return type == detail::TimeType::NEG_INFINITY; };
    bool is_pos_infinity() const { return type == detail::TimeType::POS_INFINITY; };
    bool is_special() const { return type != detail::TimeType::NORMAL; };

    void advance(const Fmi::TimeDuration& td);

    inline LocalDateTime& operator += (const Fmi::TimeDuration& td) { advance(td); return *this; }
    inline LocalDateTime& operator -= (const Fmi::TimeDuration& td) { advance(-td); return *this; }

    bool operator < (const LocalDateTime& other) const;
    bool operator > (const LocalDateTime& other) const;
    bool operator == (const LocalDateTime& other) const;

  private:
    detail::TimeType type;
  };

  std::string to_iso_string(const LocalDateTime& time);
  std::string to_iso_extended_string(const LocalDateTime& time);

  LocalDateTime operator + (const LocalDateTime& time, const Fmi::TimeDuration& td);
  LocalDateTime operator - (const LocalDateTime& time, const Fmi::TimeDuration& td);
  TimeDuration operator - (const LocalDateTime& to, const LocalDateTime& from);
  
  inline std::ostream& operator << (std::ostream& os, const LocalDateTime& time)
  {
    time.printOn(os);
    return os;
  }

  //using LocalDateTime = boost::local_time::local_date_time;
  //using TimeZonePtr = boost::local_time::time_zone_ptr;
}
