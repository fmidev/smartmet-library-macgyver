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
#include "Exception.h"
#include <array>
#include <cmath>
#include <vector>

/*=== Public interface =====================*/

namespace Fmi
{
namespace Astronomy
{
/*
 * Calculate sunrise, sunset and noon times on a certain day
 */
solar_time_t solar_time(const Fmi::LocalDateTime& ldt, double lon_e, double lat)
{
  try
  {
    std::array<JulianTime, 2> jt;

    check_lonlat(lon_e, lat);

    // *** Find the time of solar noon at the location.
    //
    JulianTime noon = SolNoon(ldt, lon_e);

    /*
     * Find prior and next sunrise/sunset, at the given location on Earth.
     *
     * Returns: Vector of one time (current day) if it has a sunrise/set
     *          Vector of two times (prior and next) if current day has no sunrise/set
     */
    jt[0] = Sunrise_or_set_UTC(noon, lon_e, lat, true /*sunrise*/);
    jt[1] = Sunrise_or_set_UTC(noon, lon_e, lat, false /*sunset*/);

    /* No sunrise/sunset in the day?
     */
    for (int i = 0; i < 2; i++)
    {
      if (jt[i].valid())
        continue;  // OK

      unsigned doy = ldt.date().day_of_year();

      // Northern hemisphere spring or summer, OR
      // Southern hemisphere fall or winter
      //

      // Arctic/Antarctic circles not explicitly present
      // since refraction and other things are taken into account
      bool all_day =
          ((lat > 0) && (doy > 79) && (doy < 267)) || ((lat <= 0) && ((doy < 83) || (doy > 263)));

      JulianTime J(noon);
      do
      {
        if ((i == 0) ? all_day : !all_day)
          --J;
        else
          ++J;

        jt[i] = Sunrise_or_set_UTC(J, lon_e, lat, i == 0 /*sunrise/sunset*/);

      } while (jt[i].JulianDay() == 0.0);
    }

    auto tz = ldt.zone();
    return solar_time_t(jt[0].ldt(tz), jt[1].ldt(tz), noon.ldt(tz));
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

/*
 * Calculate daylength. The code assumes that DST changes do not occur
 * at the given timezone during polar nights or midnight sun events
 * so that a day is either 0 or 24 hours long in local time.
 */

TimeDuration solar_time_t::daylength() const
{
  try
  {
    if (sunrise_today())
    {
      if (sunset_today())
        return sunset - sunrise;

      return Fmi::Hours(24) - sunrise.local_time().time_of_day();
    }
    if (sunset_today())
      return sunset.local_time().time_of_day();
    if (polar_night())
      return Fmi::Seconds(0);

    return Fmi::Hours(24);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace Astronomy
}  // namespace Fmi

// ======================================================================
