/*
 * Solar calculations
 *
 * Based on NOAA JavaScript at
 *       <http://www.srrb.noaa.gov/highlights/sunrise/azel.html>
 *       <http://www.srrb.noaa.gov/highlights/sunrise/sunrise.html>
 *
 * Transformation to C++ and Boost 1.3.5 by Ilmatieteen Laitos, 2008.
 *
 * Reference:
 *       Solar Calculation Details
 *       <http://www.srrb.noaa.gov/highlights/sunrise/calcdetails.html>
 *
 *       Wikipedia: Julian day
 *       <http://en.wikipedia.org/wiki/Julian_day>
 *
 * License:
 *       UNKNOWN (not stated in JavaScript)
 */
#include "Astronomy.h"
#include "AstronomyHelperFunctions.h"
#include "AstronomyJulianTime.h"

#include <vector>
#include <cmath>

using namespace std;
using boost::posix_time::not_a_date_time;
using boost::gregorian::date;
using boost::local_time::time_zone_ptr;
using boost::local_time::local_date_time;
using boost::posix_time::ptime;

/*=== Public interface =====================*/

namespace Fmi
{
namespace Astronomy
{
/*
 * Calculate position of Sun, as seen from a location on Earth
 *
 * 'utc':        Time in UTC
 * 'lat', 'lon_e': Position on Earth
 */
solar_position_t solar_position(const ptime& utc, double lon_e, double lat)
{
  check_lonlat(lon_e, lat);

  JulianTime J(utc);
  //
  // 2008-Jun-18: 2454635.5

  double declination = J.SunDeclination();  // degrees
  double eqtime = J.EquationOfTime();

  double trueSolarTime =
      (utc.time_of_day().total_seconds()) / 60.0 + eqtime + (lon_e * 4.0);  // minutes

  while (trueSolarTime > 1440)
  {
    trueSolarTime -= 1440;
  }

  double hourAngle = trueSolarTime / 4.0 - 180.0;
  if (hourAngle < -180.0) hourAngle += 360.0;

  double csz = sin_deg(lat) * sin_deg(declination) +
               cos_deg(lat) * cos_deg(declination) * cos_deg(hourAngle);

  clamp_to(csz, -1.0, 1.0);

  double zenith = rad2deg(acos(csz));

  double azDenom = cos_deg(lat) * sin_deg(zenith);
  double azimuth;

  if (fabs(azDenom) <= 0.001)
  {
    azimuth = (lat > 0.0) ? 180.0 : 0.0;
  }
  else
  {
    double azRad = ((sin_deg(lat) * cos_deg(zenith)) - sin_deg(declination)) / azDenom;
    clamp_to(azRad, -1.0, 1.0);

    azimuth = 180.0 - rad2deg(acos(azRad));

    if (hourAngle > 0.0) azimuth = -azimuth;

    if (azimuth < 0.0) azimuth += 360.0;
  }

  double refractionCorrection;

  double exoatmElevation = 90.0 - zenith;
  if (exoatmElevation <= 85.0)
  {
    double te = tan_deg(exoatmElevation);
    if (exoatmElevation > 5.0)
    {
      refractionCorrection =
          58.1 / te - 0.07 / (te * te * te) + 0.000086 / (te * te * te * te * te);
    }
    else if (exoatmElevation > -0.575)
    {
      refractionCorrection =
          1735.0 +
          exoatmElevation *
              (-518.2 +
               exoatmElevation * (103.4 + exoatmElevation * (-12.79 + exoatmElevation * 0.711)));
    }
    else
    {
      refractionCorrection = -20.774 / te;
    }
    refractionCorrection /= 3600.0;
  }
  else
  {
    refractionCorrection = 0.0;
  }

  double solarZen = zenith - refractionCorrection;
  double elevation = 90.0 - solarZen;

  return solar_position_t(azimuth, declination, elevation);
}

/*
 * Calculate position of Sun, as seen from a location on Earth
 *
 * 'lt':     Local time on Earth (includes time zone info etc.)
 * 'lon_e', 'lat': Position on Earth
 */
solar_position_t solar_position(const local_date_time& ldt, double lon_e, double lat)
{
  return solar_position(ldt.utc_time(), lon_e, lat);
}

}  // namespace Astronomy
}  // namespace Fmi

// ======================================================================
