// ======================================================================
/*!
 * \brief Astronomical calculations
 *
 * Solar calculations
 *
 * Based on NOAA JavaScript at
 *       <http://www.srrb.noaa.gov/highlights/sunrise/azel.html>
 *       <http://www.srrb.noaa.gov/highlights/sunrise/sunrise.html>
 *
 * Reference:
 *       Solar Calculation Details
 *       <http://www.srrb.noaa.gov/highlights/sunrise/calcdetails.html>
 *
 * Transformation to C++ by Ilmatieteen Laitos, 2008.
 *
 * License:
 *       UNKNOWN (not stated in JavaScript)
 */
// ======================================================================

#pragma once

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>

namespace Fmi
{
namespace Astronomy
{
/*
 * Position of Sun at a given time and place on Earth
 *
 * 'lon': Positive east of GM [-180,180]
 * 'lat': [-90,90]; truncated to +-89.8 near the poles
 *
 * To give times in UTC, create a 'local_date_time' with UTC timezone.
 */

struct solar_position_t
{
  const double azimuth;      // solar azimuth
  const double declination;  // solar declination (degrees)
  const double elevation;    // solar elevation

  solar_position_t(double a, double d, double e) : azimuth(a), declination(d), elevation(e) {}
  bool dark() const { return (elevation < -0.0145386); }
};

/*
 * Sunrise, noon and sunset times
 *
 * 'lon': Positive east of GM [-180,180]
 * 'lat': [-90,90]; truncated to +-89.8 near the poles
 *
 * Near the poles, sun may never (within a day) rise or set. In such a case,
 * rise and set are set to the TWO NEAREST EVENTS of the noon (one prior,
 * one after). 'sunset' may precede 'sunrise', which means we're in all night
 * season.
 */

struct solar_time_t
{
  const boost::local_time::local_date_time sunrise;
  const boost::local_time::local_date_time sunset;
  const boost::local_time::local_date_time noon;  // even if below the horizon

  bool sunrise_today() const { return sunrise.local_time().date() == noon.local_time().date(); }
  bool sunset_today() const { return sunset.local_time().date() == noon.local_time().date(); }
  bool polar_day() const { return sunset.local_time().date() > sunrise.local_time().date(); }
  bool polar_night() const { return sunset < sunrise; }
  solar_time_t(const boost::local_time::local_date_time& sr,
               const boost::local_time::local_date_time& ss,
               const boost::local_time::local_date_time& noon_)
      : sunrise(sr), sunset(ss), noon(noon_)
  {
  }

  boost::posix_time::time_duration daylength() const;
};

enum SetAndRiseOccurence
{
  FIRST_RISE,
  SECOND_RISE,
  FIRST_SET,
  SECOND_SET
};

struct lunar_time_t
{
  boost::local_time::local_date_time moonrise;
  boost::local_time::local_date_time moonset;
  boost::local_time::local_date_time moonrise2;
  boost::local_time::local_date_time moonset2;
  bool rise_today;
  bool set_today;
  bool rise2_today;
  bool set2_today;
  bool above_hz_24h;

  lunar_time_t(const boost::local_time::local_date_time& mr,
               const boost::local_time::local_date_time& ms,
               const boost::local_time::local_date_time& mr2,
               const boost::local_time::local_date_time& ms2,
               bool rise,
               bool set,
               bool rise2,
               bool set2,
               bool above24h)
      : moonrise(mr),
        moonset(ms),
        moonrise2(mr2),
        moonset2(ms2),
        rise_today(rise),
        set_today(set),
        rise2_today(rise2),
        set2_today(set2),
        above_hz_24h(above24h)
  {
  }

  lunar_time_t()
      : moonrise(boost::local_time::not_a_date_time),
        moonset(boost::local_time::not_a_date_time),
        moonrise2(boost::local_time::not_a_date_time),
        moonset2(boost::local_time::not_a_date_time),
        rise_today(false),
        set_today(false),
        rise2_today(false),
        set2_today(false),
        above_hz_24h(false)
  {
  }

  const boost::local_time::local_date_time& risesettime(SetAndRiseOccurence occ) const;

  bool moonrise_today() const { return rise_today; }
  bool moonset_today() const { return set_today; }
  bool moonrise2_today() const { return rise2_today; }
  bool moonset2_today() const { return set2_today; }
  bool above_horizont_24h() const { return above_hz_24h; }
  std::string as_string(SetAndRiseOccurence occ) const;
  std::string as_string_long(SetAndRiseOccurence occ) const;  // date included
};

std::ostream& operator<<(std::ostream&, const lunar_time_t& lt);

// Actual functions

solar_position_t solar_position(const boost::local_time::local_date_time& ldt,
                                double lon,
                                double lat);

solar_position_t solar_position(const boost::posix_time::ptime& utc, double lon, double lat);

solar_time_t solar_time(const boost::gregorian::date& ldate, double lon, double lat);

solar_time_t solar_time(const boost::local_time::local_date_time& ldt, double lon, double lat);

double moonphase(const boost::posix_time::ptime& utc);

double lunar_phase(const boost::posix_time::ptime& utc);
lunar_time_t lunar_time(const boost::local_time::local_date_time& ldt,
                        double lon,
                        double lat,
                        bool allow_missing_dates = false);
std::string moon_rise(const lunar_time_t& lunar_time_t);
std::string moon_set(const lunar_time_t& lunar_time_t);
std::string moon_riseset(const lunar_time_t& lunar_time_t);

}  // namespace Astronomy
}  // namespace Fmi

// ======================================================================
