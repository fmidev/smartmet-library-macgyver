#include "Astronomy.h"
#include "AstronomyHelperFunctions.h"
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/local_time_adjustor.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>

#include <limits>
#include <vector>
#include <cmath>

using namespace std;
using boost::posix_time::time_duration;
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
namespace
{
std::vector<double> quad(double ym, double yz, double yp)
{
  double nz(0.0);
  double z1(0.0);
  double z2(0.0);
  double a = 0.5 * (ym + yp) - yz;
  double b = 0.5 * (yp - ym);
  double c = yz;
  double xe = -b / (2 * a);
  double ye = (a * xe + b) * xe + c;
  double dis = b * b - 4 * a * c;
  if (dis > 0)
  {
    double dx = 0.5 * sqrt(dis) / fabs(a);
    z1 = xe - dx;
    z2 = xe + dx;
    nz = fabs(z1) < 1 ? nz + 1 : nz;
    nz = fabs(z2) < 1 ? nz + 1 : nz;
    z1 = z1 < -1 ? z2 : z1;
  }

  std::vector<double> ret;
  ret.push_back(nz);
  ret.push_back(z1);
  ret.push_back(z2);
  ret.push_back(xe);
  ret.push_back(ye);

  return ret;
}

/**
 *      returns an angle in degrees in the range 0 to 360
 */
double degRange(double x)
{
  double b = x / 360;
  double a = 360 * (b - static_cast<int>(b));
  double retVal = (a < 0 ? a + 360 : a);
  return retVal;
}

double lmst(double mjd, double glon)
{
  double d = mjd - 51544.5;
  double t = d / 36525;
  double lst =
      degRange(280.46061839 + 360.98564736629 * d + 0.000387933 * t * t - t * t * t / 38710000);
  return lst / 15 + glon / 15;
}

/**
 *      returns the self::fractional part of x
 */
double frac(double x)
{
  x -= static_cast<int>(x);
  return x < 0 ? x + 1 : x;
}

/**
 * takes t and returns the geocentric ra and dec in an array mooneq
 * claimed good to 5' (angle) in ra and 1' in dec
 * tallies with another approximate method and with ICE for a couple of dates
 */
std::vector<double> minimoon(double t)
{
  double p2 = 6.283185307;
  double arc = 206264.8062;
  double coseps = 0.91748;
  double sineps = 0.39778;

  double lo = frac(0.606433 + 1336.855225 * t);
  double l = p2 * frac(0.374897 + 1325.552410 * t);
  double l2 = l * 2;
  double ls = p2 * frac(0.993133 + 99.997361 * t);
  double d = p2 * frac(0.827361 + 1236.853086 * t);
  double d2 = d * 2;
  double f = p2 * frac(0.259086 + 1342.227825 * t);
  double f2 = f * 2;

  double sinls = sin(ls);
  double sinf2 = sin(f2);

  double dl = 22640 * sin(l);
  dl += -4586 * sin(l - d2);
  dl += 2370 * sin(d2);
  dl += 769 * sin(l2);
  dl += -668 * sinls;
  dl += -412 * sinf2;
  dl += -212 * sin(l2 - d2);
  dl += -206 * sin(l + ls - d2);
  dl += 192 * sin(l + d2);
  dl += -165 * sin(ls - d2);
  dl += -125 * sin(d);
  dl += -110 * sin(l + ls);
  dl += 148 * sin(l - ls);
  dl += -55 * sin(f2 - d2);

  double s = f + (dl + 412 * sinf2 + 541 * sinls) / arc;
  double h = f - d2;
  double n = -526 * sin(h);
  n += 44 * sin(l + h);
  n += -31 * sin(-l + h);
  n += -23 * sin(ls + h);
  n += 11 * sin(-ls + h);
  n += -25 * sin(-l2 + f);
  n += 21 * sin(-l + f);

  double L_moon = p2 * frac(lo + dl / 1296000);
  double B_moon = (18520.0 * sin(s) + n) / arc;

  double cb = cos(B_moon);
  double x = cb * cos(L_moon);
  double v = cb * sin(L_moon);
  double w = sin(B_moon);
  double y = coseps * v - sineps * w;
  double z = sineps * v + coseps * w;
  double rho = sqrt(1 - z * z);
  double dec = (360 / p2) * atan(z / rho);
  double ra = (48 / p2) * atan(y / (x + rho));
  ra = ra < 0 ? ra + 24 : ra;

  std::vector<double> retval;
  retval.push_back(dec);
  retval.push_back(ra);

  return retval;
}

double sinAlt(double mjd, double hour, double glon, double cglat, double sglat)
{
  mjd += (hour / 24.0);
  double t = (mjd - 51544.5) / 36525;
  std::vector<double> objpos = minimoon(t);

  double ra = objpos[1];
  double dec = objpos[0];
  double decRad = deg2rad(dec);
  double tau = 15 * (lmst(mjd, glon) - ra);

  return sglat * sin(decRad) + cglat * cos(decRad) * cos(deg2rad(tau));
}

}  // namespace

const boost::local_time::local_date_time& lunar_time_t::risesettime(SetAndRiseOccurence occ) const
{
  if (occ == FIRST_RISE)
    return moonrise;
  else if (occ == SECOND_RISE)
    return moonrise2;
  else if (occ == FIRST_SET)
    return moonset;
  else
    return moonset2;
}

std::string lunar_time_t::as_string(SetAndRiseOccurence occ) const
{
  std::stringstream ss;

  bool rise_occurence(occ == FIRST_RISE || occ == SECOND_RISE);

  const boost::local_time::local_date_time& occ_ldt =
      (rise_occurence ? (occ == FIRST_RISE ? moonrise : moonrise2)
                      : (occ == FIRST_SET ? moonset : moonset2));

  if (!occ_ldt.is_not_a_date_time())
  {
    ss << std::setfill('0') << std::setw(2) << occ_ldt.local_time().time_of_day().hours()
       << std::setfill('0') << std::setw(2) << occ_ldt.local_time().time_of_day().minutes();
  }

  return ss.str();
}

std::string lunar_time_t::as_string_long(SetAndRiseOccurence occ) const
{
  std::stringstream ss;

  bool rise_occurence(occ == FIRST_RISE || occ == SECOND_RISE);

  const boost::local_time::local_date_time& occ_ldt =
      (rise_occurence ? (occ == FIRST_RISE ? moonrise : moonrise2)
                      : (occ == FIRST_SET ? moonset : moonset2));

  cout << occ_ldt;

  if (occ_ldt.is_not_a_date_time())
    ss << occ_ldt;
  else
    ss << occ_ldt.date() << " " << as_string(occ);

  return ss.str();
}

std::ostream& operator<<(std::ostream& ostream, const lunar_time_t& lt)
{
  std::string risestr(lt.as_string(FIRST_RISE));
  std::string setstr(lt.as_string(FIRST_SET));

  if (risestr.empty())
  {
    if (!lt.moonset_today())
    {
      if (lt.above_hz_24h)
        risestr = "****";
      else
        risestr = "----";
    }
    else
    {
      risestr = "    ";
    }
  }
  if (setstr.empty())
  {
    if (!lt.moonrise_today())
    {
      if (lt.above_hz_24h)
        setstr = "****";
      else
        setstr = "----";
    }
    else
    {
      setstr = "    ";
    }
  }

  ostream << risestr << " " << setstr;

  return ostream;
}

bool dst_on(const ptime& theTime, const boost::local_time::local_date_time& ldt)
{
  time_zone_ptr zone = ldt.zone();

  bool dst_on(false);
  if (zone->has_dst())
  {
    ptime dst_starttime(zone->dst_local_start_time(ldt.local_time().date().year()));
    ptime dst_endtime(zone->dst_local_end_time(ldt.local_time().date().year()));

    if (dst_starttime < dst_endtime)
    {
      if (theTime >= dst_starttime && theTime <= dst_endtime) dst_on = true;
    }
    else
    {
      if (theTime >= dst_starttime || theTime <= dst_endtime) dst_on = true;
    }
  }
  return dst_on;
}

double timezone_offset(const boost::local_time::local_date_time& ldt)
{
  time_zone_ptr zone = ldt.zone();

  double base_offset(zone->base_utc_offset().hours() +
                     (static_cast<float>(zone->base_utc_offset().minutes()) / 60.0));
  double dst_offset(dst_on(ldt.local_time(), ldt)
                        ? (zone->dst_offset().hours() + zone->dst_offset().minutes())
                        : 0.0);

  return (base_offset + dst_offset);
}

void get_hours_and_minutes(double hours, int& hr, int& min)
{
  double hrs(hours);

  // if time is greater than 23:59:29 use value 23:59, so that we do not move to 00:00 (backwards)
  if (hrs > 23.991 && hrs < 24.0) hrs = 23.991;

  hr = static_cast<int>(floor(hrs));
  min = static_cast<int>(round(60.0 * (hrs - hr)));
}

local_date_time parse_local_date_time(const boost::local_time::local_date_time& ldt, double hours)
{
  double hrs(hours);
  int hour(0);
  int min(0);

  get_hours_and_minutes(hrs, hour, min);
  // utc time of the beginning of the day
  ptime utc_ptime(ldt.utc_time());
  // add hour offset
  utc_ptime += time_duration(hour, min, 0, 0);

  // return local time
  local_date_time ldt_riseset(utc_ptime, ldt.zone());

  return ldt_riseset;
}

/*
 * Calculate moonrise, moonset for given location
 */
lunar_time_t lunar_time_calculation(const boost::local_time::local_date_time& ldt,
                                    double offset,
                                    double lon,
                                    double lat)
{
  double utrise(0.0);
  double utset(0.0);
  double utrise2(0.0);
  double utset2(0.0);

  // beginning of the day
  local_date_time ldt_beg(ldt.local_time().date(),
                          time_duration(0, 0, 0, 0),
                          ldt.zone(),
                          local_date_time::NOT_DATE_TIME_ON_ERROR);

  double date = ldt_beg.local_time().date().modjulian_day();

  date -= (static_cast<float>(offset) / 24.0);

  double latRad = deg2rad(lat);
  double sinho = 0.0023271056;
  double sglat = sin(latRad);
  double cglat = cos(latRad);

  bool rise = false;
  bool set = false;
  bool rise2 = false;
  bool set2 = false;
  bool above = false;
  double hour = 1.0;

  double ym = sinAlt(date, hour - 1.0, lon, cglat, sglat) - sinho;

  above = (ym > 0);
  while (hour < 25)
  {
    double yz = sinAlt(date, hour, lon, cglat, sglat) - sinho;
    double yp = sinAlt(date, hour + 1.0, lon, cglat, sglat) - sinho;

    std::vector<double> quadout = quad(ym, yz, yp);
    double nz = quadout[0];
    double z1 = quadout[1];
    double z2 = quadout[2];
    // double xe = quadout[3];
    double ye = quadout[4];

    if (nz == 1)
    {
      if (ym < 0)
      {
        if (false == set || false == rise)
        {
          utrise = hour + z1;
          rise = true;
        }
        else
        {
          utrise2 = hour + z1;
          rise2 = true;
        }
      }
      else
      {
        if (false == set || false == rise)
        {
          utset = hour + z1;
          set = true;
        }
        else
        {
          utset2 = hour + z1;
          set2 = true;
        }
      }
    }

    if (nz == 2)
    {
      if (ye < 0)
      {
        if (false == set || false == rise)
        {
          utrise = hour + z2;
          utset = hour + z1;
        }
        else
        {
          utrise2 = hour + z2;
          utset2 = hour + z1;
        }
      }
      else
      {
        if (false == set || false == rise)
        {
          utrise = hour + z1;
          utset = hour + z2;
        }
        else
        {
          utrise2 = hour + z1;
          utset2 = hour + z2;
        }
      }
    }
#ifdef MYDEBUG
    std::cout << "nz: " << nz << endl;
    std::cout << "z1: " << z1 << endl;
    std::cout << "z2: " << z2 << endl;
    // std::cout << "xe: " << xe << endl;
    std::cout << "ye: " << ye << endl;
    std::cout << "utrise: " << utrise << endl;
    std::cout << "utset: " << utset << endl;
    std::cout << "utrise2: " << utrise2 << endl;
    std::cout << "utset2: " << utset2 << endl;
    std::cout << "rise: " << rise << endl;
    std::cout << "rise2: " << rise2 << endl;
    std::cout << "set: " << set << endl;
    std::cout << "set2: " << set2 << endl << endl;
#endif

    ym = yp;
    hour += 2.0;
  }

#ifdef MYDEBUG
  cout << "\noffset: " << offset << endl;
  cout << "rise: " << rise << endl;
  cout << "rise2: " << rise2 << endl;
  cout << "utrise: " << utrise << endl;
  cout << "utrise2: " << utrise2 << endl;
  cout << "set: " << set << endl;
  cout << "set2: " << set2 << endl;
  cout << "utset: " << utset << endl;
  cout << "utset2: " << utset2 << endl;
  cout << "julian: " << date << endl;
  cout << "ldt: " << ldt << endl;
  cout << "ldt.local_time(): " << ldt.local_time() << endl;
  cout << "ldt.utc_time(): " << ldt.utc_time() << endl;
  cout << "ldt_beg: " << ldt_beg << endl;
#endif

  lunar_time_t retval(
      (rise ? parse_local_date_time(ldt_beg, utrise) : local_date_time(not_a_date_time)),
      (set ? parse_local_date_time(ldt_beg, utset) : local_date_time(not_a_date_time)),
      (rise2 ? parse_local_date_time(ldt_beg, utrise2) : local_date_time(not_a_date_time)),
      (set2 ? parse_local_date_time(ldt_beg, utset2) : local_date_time(not_a_date_time)),
      rise,
      set,
      rise2,
      set2,
      (!rise && !set && above));

  return retval;
}

lunar_time_t lunar_time_i(const boost::local_time::local_date_time& ldt, double lon, double lat)
{
  // beginning of the day
  local_date_time ldt_beg(ldt.local_time().date(),
                          time_duration(0, 0, 0, 0),
                          ldt.zone(),
                          local_date_time::NOT_DATE_TIME_ON_ERROR);
  local_date_time ldt_end(ldt.local_time().date(),
                          time_duration(23, 59, 59, 0),
                          ldt.zone(),
                          local_date_time::NOT_DATE_TIME_ON_ERROR);

  ptime dst_endtime(ldt.zone()->dst_local_end_time(ldt.local_time().date().year()));
  bool dst_ends_today(ldt_beg.local_time().date() == dst_endtime.date());

  double offset_before_dst_ends = timezone_offset(ldt_beg);

  lunar_time_t retval;

  // if dst ends today we have to handle the extended time
  if (dst_ends_today)
  {
    double offset_after_dst_ends = timezone_offset(ldt_end);

    lunar_time_t lt_before = lunar_time_calculation(ldt_beg, offset_before_dst_ends, lon, lat);

    // if rise and set is found return
    if (lt_before.moonrise_today() && lt_before.moonset_today()) return lt_before;

    // do calculation using wintertime offset
    lunar_time_t lt_after = lunar_time_calculation(ldt_beg, offset_after_dst_ends, lon, lat);
    if (lt_after.moonrise_today()) lt_after.moonrise += boost::posix_time::hours(1);
    if (lt_after.moonset_today()) lt_after.moonset += boost::posix_time::hours(1);
    if (lt_after.moonrise2_today()) lt_after.moonrise2 += boost::posix_time::hours(1);
    if (lt_after.moonset2_today()) lt_after.moonset2 += boost::posix_time::hours(1);

    lunar_time_t lt_combined(lt_before.moonrise_today() ? lt_before.moonrise : lt_after.moonrise,
                             lt_before.moonset_today() ? lt_before.moonset : lt_after.moonset,
                             lt_before.moonrise2_today() ? lt_before.moonrise2 : lt_after.moonrise2,
                             lt_before.moonset2_today() ? lt_before.moonset2 : lt_after.moonset2,
                             lt_before.rise_today,
                             lt_before.set_today,
                             lt_before.rise2_today,
                             lt_before.set2_today,
                             lt_before.above_hz_24h && lt_after.above_hz_24h);

    retval = lt_combined;
  }
  else
  {
    retval = lunar_time_calculation(ldt_beg, offset_before_dst_ends, lon, lat);
  }
  return retval;
}

lunar_time_t lunar_time(const boost::local_time::local_date_time& ldt,
                        double lon,
                        double lat,
                        bool allow_missing_dates /*= false*/)
{
  lunar_time_t lt_ret = lunar_time_i(ldt, lon, lat);

  if (allow_missing_dates) return lt_ret;

  unsigned int iteration_limit(366);
  if (!lt_ret.moonrise_today())
  {
    unsigned int counter(0);  // just in case something goes wrong
    boost::local_time::local_date_time ldt_iter(ldt);
    if (lt_ret.above_hz_24h || lt_ret.moonset_today())
    {
      // find the previous moonrise
      lunar_time_t lt_prev;
      while (!lt_prev.moonrise_today() && counter < iteration_limit)
      {
        ldt_iter -= boost::posix_time::hours(24);
        lt_prev = lunar_time_i(ldt_iter, lon, lat);
        counter++;
      }
      if (counter < iteration_limit) lt_ret.moonrise = lt_prev.moonrise;
    }
    else
    {
      // find the next moonrise
      lunar_time_t lt_next;
      while (!lt_next.moonrise_today() && counter < iteration_limit)
      {
        ldt_iter += boost::posix_time::hours(24);
        lt_next = lunar_time_i(ldt_iter, lon, lat);
        counter++;
      }
      if (counter < iteration_limit) lt_ret.moonrise = lt_next.moonrise;
    }
  }
  if (!lt_ret.moonset_today())
  {
    unsigned int counter(0);  // just in case something goes wrong
    boost::local_time::local_date_time ldt_iter(ldt);
    if (lt_ret.above_hz_24h || lt_ret.moonrise_today())
    {
      // find the next moonset
      lunar_time_t lt_next;
      while (!lt_next.moonset_today() && counter < iteration_limit)
      {
        ldt_iter += boost::posix_time::hours(24);
        lt_next = lunar_time_i(ldt_iter, lon, lat);
        counter++;
      }
      if (counter < iteration_limit) lt_ret.moonset = lt_next.moonset;
    }
    else
    {
      // find the previous moonset
      lunar_time_t lt_prev;
      while (!lt_prev.moonset_today() && counter < iteration_limit)
      {
        ldt_iter -= boost::posix_time::hours(24);
        lt_prev = lunar_time_i(ldt_iter, lon, lat);
        counter++;
      }
      if (counter < iteration_limit) lt_ret.moonset = lt_prev.moonset;
    }
  }
  return lt_ret;
}
}  // namespace Astronomy
}  // namespace Fmi

// ======================================================================
