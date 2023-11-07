/*
 * Solar calculations - selftests
 */

#include "Astronomy.h"
#include "TimeParser.h"
#include <iostream>

using namespace std;

using namespace Fmi::Astronomy;
using namespace boost::posix_time;
using namespace boost::gregorian;

// ptime, time_duration, not_a_time_date

#define _LATLON(deg, min, sec) (((double)(deg)) + (min) / 60.0 + (sec) / 3600.0)

#define LON_E _LATLON
#define LON_W(d, m, s) (-_LATLON(d, m, s))

#define LAT_N _LATLON
#define LAT_S(d, m, s) (-_LATLON(d, m, s))

struct test_data_t
{
  const char *title;
  const char *tz_str;  // time zone id
  double lon, lat;     // longitude/latitude

  Fmi::DateTime when;  // Fmi::Date & time in LOCAL time
  double right_decl;
  double right_azim;
  double right_elev;

  Fmi::DateTime right_sunrise;
  Fmi::DateTime right_sunset;
  Fmi::TimeDuration right_noon;
  Fmi::TimeDuration daylen;

  /* Any better way to do this?
   */
  test_data_t(const char *_title,
              const char *_tz_str,
              double _lon,
              double _lat,
              Fmi::DateTime _when,
              double d,
              double a,
              double e,
              Fmi::DateTime sr,
              Fmi::DateTime ss,
              const Fmi::TimeDuration &noon,
              const Fmi::TimeDuration &_daylen)
      : title(_title),
        tz_str(_tz_str),
        lon(_lon),
        lat(_lat),
        when(_when),
        right_decl(d),
        right_azim(a),
        right_elev(e),
        right_sunrise(sr),
        right_sunset(ss),
        right_noon(noon),
        daylen(_daylen)
  {
  }
};

static vector<test_data_t> test_data;

#define ZONESPEC_CVS "/usr/share/smartmet/timezones/date_time_zonespec.csv"

boost::shared_ptr<boost::local_time::tz_database> tz_db;

static Fmi::TimeZonePtr tz_from_region(const string &id)
{
  if (tz_db == 0)
  {
    tz_db.reset(new boost::local_time::tz_database());
    tz_db->load_from_file(ZONESPEC_CVS);
  }

  return tz_db->time_zone_from_region(id);
}

static void prepare_test_data()
{
  const Fmi::Date JUN_18_2008(2008, 6, 18);
  const Fmi::Date DEC_31_2012(2012, 12, 31);
  const Fmi::Date APR_4_1970(1970, 4, 4);
  const Fmi::Date MAY_16_2008(2008, 5, 16);
  const Fmi::Date JUL_28_2008(2008, 7, 28);
  const Fmi::Date NOV_25_2012(2012, 11, 25);
  const Fmi::Date JAN_16_2013(2013, 1, 16);
  const Fmi::Date JUN_14_2012(2012, 6, 14);
  const Fmi::Date JUN_11_2012(2012, 6, 11);
  const Fmi::Date JUL_01_2012(2012, 7, 1);
  const Fmi::Date OCT_7_2013(2013, 10, 7);
  const Fmi::Date JUN_18_2019(2019, 6, 18);

  /* HELSINKI 2008-Jun-18 15:34:07
   */
  test_data.push_back(test_data_t("Helsinki",
                                  "Europe/Helsinki",
                                  LON_E(24, 58, 0),
                                  LAT_N(60, 10, 0),
                                  //
                                  Fmi::DateTime(JUN_18_2008, Fmi::TimeDuration(15, 34, 7)),
                                  23.4191,   // declination
                                  227.0028,  // azimuth
                                  46.6335,   // elevation
                                  //
                                  Fmi::DateTime(JUN_18_2008, Fmi::TimeDuration(3, 53, 42)),
                                  Fmi::DateTime(JUN_18_2008, Fmi::TimeDuration(22, 49, 9)),
                                  Fmi::TimeDuration(13, 21, 13),
                                  Fmi::TimeDuration(18, 55, 27)));

  /* Utsjoki kesäaurinko / kaamos / jotain väliltä
   */
  test_data.push_back(
      test_data_t("Utsjoki 1",
                  "Europe/Helsinki",
                  LON_E(27, 1, 25),
                  LAT_N(69, 54, 22),
                  //
                  Fmi::DateTime(JUN_18_2008, Fmi::TimeDuration(13, 0, 0)),
                  23.41723981574298,  // declination
                  175.8591,           // azimuth
                  43.4874,            // elevation
                  //
                  Fmi::DateTime(Fmi::Date(2008, 5, 16), Fmi::TimeDuration(1, 36, 13)),
                  Fmi::DateTime(Fmi::Date(2008, 7, 28), Fmi::TimeDuration(0, 37, 33)),
                  Fmi::TimeDuration(13, 12, 59),
                  Fmi::Hours(24)));

  test_data.push_back(
      test_data_t("Utsjoki 2",
                  "Europe/Helsinki",
                  LON_E(27, 1, 25),
                  LAT_N(69, 54, 22),
                  //
                  Fmi::DateTime(DEC_31_2012, Fmi::TimeDuration(13, 0, 0)),
                  -23.045737369904682,  // declination
                  190.34,               // azimuth
                  -3.1991,              // elevation
                  //
                  Fmi::DateTime(Fmi::Date(2013, 1, 16), Fmi::TimeDuration(12, 2, 26)),
                  Fmi::DateTime(Fmi::Date(2012, 11, 25), Fmi::TimeDuration(12, 18, 16)),
                  Fmi::TimeDuration(),  // (12,14,49) but NOAA does not give it so not among the test
                  Fmi::Seconds(0)));

  // Aurinko nousee kes�ksi
  test_data.push_back(
      test_data_t("Utsjoki 3",
                  "Europe/Helsinki",
                  LON_E(27, 1, 25),
                  LAT_N(69, 54, 22),
                  //
                  Fmi::DateTime(MAY_16_2008, Fmi::TimeDuration(13, 0, 0)),
                  19.2338,  // declination
                  177.476,  // azimuth
                  39.3317,  // elevation
                  //
                  Fmi::DateTime(Fmi::Date(2008, 5, 16), Fmi::TimeDuration(1, 36, 13)),
                  Fmi::DateTime(Fmi::Date(2008, 7, 28), Fmi::TimeDuration(0, 37, 33)),
                  Fmi::TimeDuration(13, 8, 15),
                  Fmi::TimeDuration(22, 23, 47)));

  // Ja laskee hein�kuussa
  test_data.push_back(
      test_data_t("Utsjoki 4",
                  "Europe/Helsinki",
                  LON_E(27, 1, 25),
                  LAT_N(69, 54, 22),
                  //
                  Fmi::DateTime(JUL_28_2008, Fmi::TimeDuration(13, 0, 0)),
                  18.8471,  // declination
                  174.407,  // azimuth
                  38.8839,  // elevation
                  //
                  Fmi::DateTime(Fmi::Date(2008, 7, 28), Fmi::TimeDuration(2, 0, 9)),
                  Fmi::DateTime(Fmi::Date(2008, 7, 29), Fmi::TimeDuration(0, 23, 46)),
                  Fmi::TimeDuration(13, 18, 24),
                  Fmi::TimeDuration(21, 59, 51)));

  // Aurinko laskee talveksi
  test_data.push_back(
      test_data_t("Utsjoki 5",
                  "Europe/Helsinki",
                  LON_E(27, 1, 25),
                  LAT_N(69, 54, 22),
                  //
                  Fmi::DateTime(NOV_25_2012, Fmi::TimeDuration(13, 0, 0)),
                  -20.8674,  // declination
                  194.23,    // azimuth
                  -1.18855,  // elevation
                  //
                  Fmi::DateTime(Fmi::Date(2012, 11, 25), Fmi::TimeDuration(11, 38, 57)),
                  Fmi::DateTime(Fmi::Date(2012, 11, 25), Fmi::TimeDuration(12, 18, 15)),
                  Fmi::TimeDuration(11, 58, 50),
                  Fmi::TimeDuration(0, 39, 18)));

  // Ja nousee tammikuussa
  test_data.push_back(
      test_data_t("Utsjoki 6",
                  "Europe/Helsinki",
                  LON_E(27, 1, 25),
                  LAT_N(69, 54, 22),
                  //
                  Fmi::DateTime(JAN_16_2013, Fmi::TimeDuration(13, 0, 0)),
                  -20.8541,   // declination
                  188.938,    // azimuth
                  -0.690872,  // elevation
                  //
                  Fmi::DateTime(Fmi::Date(2013, 1, 16), Fmi::TimeDuration(12, 2, 26)),
                  Fmi::DateTime(Fmi::Date(2013, 1, 16), Fmi::TimeDuration(12, 41, 45)),
                  Fmi::TimeDuration(12, 21, 32),
                  Fmi::TimeDuration(0, 39, 19)));

  /* Boost DST does NOT work right with historic Fmi::Dates; Finland did NOT
   * have DST in 1970 (or the CSV file does not reflect this).
   *
   * If 'have DST' is set in NOAA page, the results match.
   */

#if 0
    test_data.push_back( test_data_t(
          "Utsjoki 0", 
          "Europe/Helsinki", LON_E(27,1,25), LAT_N(69,54,22),
          //
          Fmi::DateTime( APR_4_1970, Fmi::TimeDuration(13,0,0) ),
          5.626002121616464,      // declination
          192.399,     // azimuth
          25.3369,      // elevation
          //
          Fmi::DateTime( APR_4_1970, Fmi::TimeDuration(5,3,54) ),
          Fmi::DateTime( APR_4_1970, Fmi::TimeDuration(19,28,48) ),
          Fmi::TimeDuration(12,15,10),
		  Fmi::TimeDuration(14,24,53)
    ));
#endif

  /* London, no DST
   */
  test_data.push_back(test_data_t("Lontoo",
                                  "Europe/London",
                                  LON_W(0, 10, 0),
                                  LAT_N(51, 30, 0),
                                  //
                                  Fmi::DateTime(DEC_31_2012, Fmi::TimeDuration(22, 5, 59)),
                                  -23.00891107988152,  // declination
                                  NAN,                 // azimuth (dark; NOAA gives no value)
                                  NAN,                 // elevation (dark; NOAA gives no value)
                                  //
                                  Fmi::DateTime(DEC_31_2012, Fmi::TimeDuration(8, 6, 22)),
                                  Fmi::DateTime(DEC_31_2012, Fmi::TimeDuration(16, 1, 33)),
                                  Fmi::TimeDuration(12, 3, 38),
                                  Fmi::TimeDuration(7, 55, 11)));

  /* Chicago, IL (-6 hrs, DST)
   */
  test_data.push_back(test_data_t("Chicago (-6)",
                                  "America/Chicago",
                                  LON_W(87, 39, 0),
                                  LAT_N(41, 51, 0),
                                  //
                                  Fmi::DateTime(JUN_18_2008, Fmi::TimeDuration(8, 0, 0)),
                                  23.419408805735358,  // declination
                                  82.4293,             // azimuth
                                  27.776,              // elevation
                                  //
                                  Fmi::DateTime(JUN_18_2008, Fmi::TimeDuration(5, 15, 11)),
                                  Fmi::DateTime(JUN_18_2008, Fmi::TimeDuration(20, 28, 35)),
                                  Fmi::TimeDuration(12, 51, 45),
                                  Fmi::TimeDuration(15, 13, 25)));

  /* Bombay, India (+5:30 hrs)
   */
  test_data.push_back(test_data_t("Mumbai (+5:30)",
                                  "Asia/Calcutta",
                                  LON_E(72, 50, 0),
                                  LAT_N(18, 56, 0),  // whole India in one time zone
                                  //
                                  Fmi::DateTime(JUN_18_2019, Fmi::TimeDuration(13, 20, 0)),
                                  23.3976,  // declination
                                  297.186,  // azimuth
                                  79.6093,  // elevation
                                  //
                                  Fmi::DateTime(JUN_18_2019, Fmi::TimeDuration(6, 1, 38)),
                                  Fmi::DateTime(JUN_18_2019, Fmi::TimeDuration(19, 17, 48)),
                                  Fmi::TimeDuration(12, 39, 36),
                                  Fmi::TimeDuration(13, 16, 10)));

  /* Kiribati (+14 time zone)
   *
   * NOTE: NOAA web pages don't take > +-12hr time zones. The reference values
   *       have been calculated with UTC setup and -1 day:
   *
   *       azel.html:
   *           Offset to UTC:      0
   *           Daylight saving:    no
   *           Date:               Jun 17th 2008
   *           Time:               18:00:00 (= 8am at UTC+14)
   */
  test_data.push_back(
      test_data_t("Kiribati (+14)",
                  "Pacific/Kiritimati",
                  LON_W(157, 42, 0),
                  LAT_N(1, 52, 0),
                  //
                  Fmi::DateTime(JUN_18_2008, Fmi::TimeDuration(8, 0, 0)),
                  23.40385438036006,  // declination
                  65.6055,            // azimuth
                  20.9652,            // elevation
                  //
                  Fmi::DateTime(JUN_18_2008, Fmi::TimeDuration(6, 24, 58)),   // NOAA says 16:24:58 UTC (17th)
                  Fmi::DateTime(JUN_18_2008, Fmi::TimeDuration(18, 38, 49)),  // NOAA says 4:38:49 UTC (18th)
                  Fmi::TimeDuration(12, 31, 47),                      // NOAA says 22:31:47 UTC (17th)
                  Fmi::TimeDuration(12, 13, 51)));

  // Kuusamo

  test_data.push_back(test_data_t("Kuusamo",
                                  "Europe/Helsinki",
                                  LON_E(29, 12, 0),
                                  LAT_N(66, 0, 0),  //
                                  //
                                  Fmi::DateTime(JUN_14_2012, Fmi::TimeDuration(13, 20, 0)),
                                  NAN,  // declination
                                  NAN,  // azimuth
                                  NAN,  // elevation
                                  //
                                  Fmi::DateTime(JUN_11_2012, Fmi::TimeDuration(1, 22, 11)),
                                  Fmi::DateTime(JUL_01_2012, Fmi::TimeDuration(0, 48, 30)),
                                  Fmi::TimeDuration(13, 03, 26),
                                  Fmi::TimeDuration(24, 0, 0)));

  /* Honolulu, HI (-10 hrs, no DST)
   */
  test_data.push_back(test_data_t("Honolulu (-10)",
                                  "Pacific/Honolulu",
                                  LON_W(157, 51, 0),
                                  LAT_N(21, 18, 0),
                                  //
                                  Fmi::DateTime(JUN_18_2008, Fmi::TimeDuration(19, 37, 0)),
                                  23.429473208579576,  // declination
                                  297.686,             // azimuth
                                  -5.2182,             // elevation
                                  //
                                  Fmi::DateTime(JUN_18_2008, Fmi::TimeDuration(5, 49, 47)),
                                  Fmi::DateTime(JUN_18_2008, Fmi::TimeDuration(19, 15, 38)),
                                  Fmi::TimeDuration(12, 32, 36),
                                  Fmi::TimeDuration(13, 25, 50)));

  /* Namaka, Fidzi (+12 hours, DST)
   */

  test_data.push_back(test_data_t("Namaka (+12)",
                                  "Pacific/Fiji",
                                  LON_E(177, 58, 59),
                                  LAT_S(17, 45, 57),
                                  //
                                  Fmi::DateTime(OCT_7_2013, Fmi::TimeDuration(8, 0, 0)),
                                  -5.41583,  // declination
                                  85.5431,   // azimuth
                                  31.1536,   // elevation
                                  //
                                  Fmi::DateTime(OCT_7_2013, Fmi::TimeDuration(5, 45, 38)),
                                  Fmi::DateTime(OCT_7_2013, Fmi::TimeDuration(18, 6, 36)),
                                  Fmi::TimeDuration(11, 56, 8),
                                  Fmi::TimeDuration(12, 20, 58)));

  /* Pago Pago, American Samoa (-11, no DST)
   */

  test_data.push_back(test_data_t("Apia (+11)",
                                  "Pacific/Pago_Pago",
                                  LON_W(170, 42, 7),
                                  LAT_S(14, 16, 41),
                                  //
                                  Fmi::DateTime(OCT_7_2013, Fmi::TimeDuration(8, 0, 0)),
                                  -5.78214,  // declination
                                  89.0152,   // azimuth
                                  27.9459,   // elevation
                                  //
                                  Fmi::DateTime(OCT_7_2013, Fmi::TimeDuration(6, 1, 11)),
                                  Fmi::DateTime(OCT_7_2013, Fmi::TimeDuration(18, 19, 55)),
                                  Fmi::TimeDuration(12, 10, 35),
                                  Fmi::TimeDuration(12, 18, 44)));
};

#define validate(a, b, factor)                                         \
  if (std::isnan(b))                                                   \
  {                                                                    \
    cerr << "NOT TESTED " << #a << ": " << a << endl;                  \
  }                                                                    \
  else if (floor((a) * (factor) + 0.5) == floor((b) * (factor) + 0.5)) \
  {                                                                    \
    cerr << "OK " << #a << ": " << a << " ~ " << b << endl;            \
  }                                                                    \
  else                                                                 \
  {                                                                    \
    cerr << "FAILED " << #a << ": " << a << " != " << b << endl;       \
    fails++;                                                           \
  }

#define validate_time(a, b)                                      \
  if (abs(((a) - (b)).total_seconds()) <= 1)                     \
  {                                                              \
    cerr << "OK " << #a << ": " << a << " ~ " << b << endl;      \
  }                                                              \
  else                                                           \
  {                                                              \
    cerr << "FAILED " << #a << ": " << a << " != " << b << endl; \
    fails++;                                                     \
  }

#define not_tested_time(a)                            \
  {                                                   \
    cerr << "NOT TESTED " << #a << ": " << a << endl; \
  }

/*
 */
int main(int argc, const char **argv)
{
  unsigned fails = 0;

  prepare_test_data();

  for (unsigned n = 0; n < test_data.size(); n++)
  {
    test_data_t *td = &test_data[n];

    cout << "\n*** " << td->title << " " << td->when << " (" << td->tz_str << ") ***\n\n";

    // For Posix time zone date, see i.e.
    //
    // <http://home.tiscali.nl/~t876506/TZworld.html>
    // <http://www.timeanddate.com/library/abbreviations/timezones/eu/eet.html>
    // <http://www.timeanddate.com/library/abbreviations/timezones/>
    //
    // <http://www.kellonaika.fi/aikavyohykkeet.php>
    // <http://en.wikipedia.org/wiki/List_of_zoneinfo_timezones>
    //

    Fmi::TimeZonePtr tz = tz_from_region(td->tz_str);

    if (!tz)
      throw runtime_error("Unknown time zone: '" + string(td->tz_str) + "'");

    //---
    // EXCEPTION_ON_ERROR: throws up if given time is non-existing, or possible
    //                      duplicate (happens in night, when DST changes)
    //

    Fmi::LocalDateTime ldt =
        Fmi::TimeParser::make_time(td->when.date(), td->when.time_of_day(), tz);

    solar_position_t sp = solar_position(ldt, td->lon, td->lat);

    validate(sp.declination, td->right_decl, 1000);
    validate(sp.azimuth, td->right_azim, 100);
    validate(sp.elevation, td->right_elev, 1000);

    //---
    solar_time_t ss = solar_time(ldt, td->lon, td->lat);

    validate_time(ss.sunrise.local_time(), td->right_sunrise);
    validate_time(ss.sunset.local_time(), td->right_sunset);

    // Skip the test if no noon given for comparison
    //
    if (td->right_noon.total_seconds() == 0)
    {
      not_tested_time(ss.noon);
    }
    else
    {
        validate_time(ss.noon.local_time(), Fmi::DateTime(td->when.date(), td->right_noon));
    }

    validate_time(ss.daylength(), td->daylen);
  }

  cout << "\n" << fails << " failures.\n";
  return fails ? -1 : 0;
}
