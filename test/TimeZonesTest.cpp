// ======================================================================
/*!
 * \file
 * \brief Regression tests for Fmi::TimeZones
 */
// ======================================================================

#include "TimeZones.h"
#include <boost/lexical_cast.hpp>
#include <regression/tframe.h>
#include <iostream>
#include <string>

using namespace std;

static Fmi::TimeZones timezones;

namespace TimeZonesTest
{
// ----------------------------------------------------------------------
/*
 * Test region list
 */
// ----------------------------------------------------------------------

void region_list()
{
  using boost::lexical_cast;

  vector<string> lst = timezones.region_list();
  if (lst.size() < 460)
    TEST_FAILED("There should at least 460 regions, not " + lexical_cast<string>(lst.size()));

  if (lst.front() != "Africa/Abidjan")
    TEST_FAILED("First region should be Africa/Abidjan");

  if (lst.back() != "Zulu")
    TEST_FAILED("Last region should be Zulu, not " + lst.back());

  TEST_PASSED();
}

// ----------------------------------------------------------------------
/*
 * Test time_zone_from_region
 */
// ----------------------------------------------------------------------

void time_zone_from_region()
{
  using boost::lexical_cast;
  using boost::local_time::time_zone_ptr;

  time_zone_ptr tz = timezones.time_zone_from_region("Europe/Helsinki");
  if (tz->dst_zone_abbrev() != "EEST")
    TEST_FAILED("Europe/Helsinki dst_zone_abbrev should be EEST, not " + tz->dst_zone_abbrev());
  if (tz->std_zone_abbrev() != "EET")
    TEST_FAILED("Europe/Helsinki std_zone_abbrev should be EET, not " + tz->std_zone_abbrev());
  if (tz->dst_zone_name() != "EEST")
    TEST_FAILED("Europe/Helsinki dst_zone_name should be EEST, not " + tz->dst_zone_name());
  if (tz->std_zone_name() != "EET")
    TEST_FAILED("Europe/Helsinki std_zone_name should be EET, not " + tz->std_zone_name());
  if (tz->has_dst() != true)
    TEST_FAILED("Europe/Helsinki is_dst should be true, not " +
                lexical_cast<string>(tz->has_dst()));

  string val;

  if ("2000-Mar-26 03:00:00" != (val = lexical_cast<string>(tz->dst_local_start_time(2000))))
    TEST_FAILED("dst_local_start_time should be 2000-Mar-26 03:00:00, not " + val);
  if ("2000-Oct-29 04:00:00" != (val = lexical_cast<string>(tz->dst_local_end_time(2000))))
    TEST_FAILED("dst_local_end_time should be 2000-Oct-29 04:00:00, not " + val);
  if ("02:00:00" != (val = lexical_cast<string>(tz->base_utc_offset())))
    TEST_FAILED("base_utc_offset should be 02:00:00, not " + val);
  if ("01:00:00" != (val = lexical_cast<string>(tz->dst_offset())))
    TEST_FAILED("dst_offset should be 01:00:00, not " + val);

  TEST_PASSED();
}

// ----------------------------------------------------------------------
/*
 * Test time_zone_from_string
 */
// ----------------------------------------------------------------------

void time_zone_from_string()
{
  using boost::lexical_cast;
  using boost::local_time::time_zone_ptr;

  // UTC

  {
    string posix = "UTC";
    time_zone_ptr tz = timezones.time_zone_from_string("UTC");

    if (tz->std_zone_name() != posix)
      TEST_FAILED("UTC string should be " + posix + ", not " + tz->std_zone_name());
  }

  // Helsinki
  {
    string region = "Europe/Helsinki";
    string posix = "EET";

    time_zone_ptr tz1 = timezones.time_zone_from_string(region);
    if (tz1->std_zone_name() != posix)
      TEST_FAILED(region + " string should be " + posix + ", not " + tz1->std_zone_name());

    time_zone_ptr tz2 = timezones.time_zone_from_string(posix);
    if (tz2->std_zone_name() != posix)
      TEST_FAILED(posix + " string should be " + posix + ", not " + tz2->std_zone_name());
  }

  TEST_PASSED();
}

// ----------------------------------------------------------------------
/*
 * Test time_zone_from_coordinate
 */
// ----------------------------------------------------------------------

void time_zone_from_coordinate()
{
  using boost::lexical_cast;
  using boost::local_time::time_zone_ptr;

  string ok1 = "CET";
  time_zone_ptr tz1 = timezones.time_zone_from_coordinate(17, 60);
  if (tz1->std_zone_name() != ok1)
    TEST_FAILED("17,60 string should be " + ok1 + ", not " + tz1->std_zone_name());

  string ok2 = "EET";
  time_zone_ptr tz2 = timezones.time_zone_from_coordinate(25, 60);
  if (tz2->std_zone_name() != ok2)
    TEST_FAILED("25,60 string should be " + ok2 + ", not " + tz2->std_zone_name());

  string ok3 = "EET";
  time_zone_ptr tz3 = timezones.time_zone_from_coordinate(21.3705, 59.7811);
  if (tz3->std_zone_name() != ok3)
    TEST_FAILED("21.3705,59.7811 string should be " + ok3 + ", not " + tz3->std_zone_name());

  TEST_PASSED();
}

// ----------------------------------------------------------------------
/*
 * Test zone_name_from_coordinate
 */
// ----------------------------------------------------------------------

void zone_name_from_coordinate()
{
  using boost::lexical_cast;
  using boost::local_time::time_zone_ptr;

  string ok1 = "Europe/Stockholm";
  string tz1 = timezones.zone_name_from_coordinate(17, 60);
  if (tz1 != ok1)
    TEST_FAILED("17,60 string should be " + ok1 + ", not " + tz1);

  string ok2 = "Europe/Helsinki";
  string tz2 = timezones.zone_name_from_coordinate(25, 60);
  if (tz2 != ok2)
    TEST_FAILED("25,60 string should be " + ok2 + ", not " + tz2);

  string ok3 = "Europe/Helsinki";
  string tz3 = timezones.zone_name_from_coordinate(21.3705, 59.7811);
  if (tz3 != ok3)
    TEST_FAILED("21.3705,59.7811 string should be " + ok3 + ", not " + tz3);

  TEST_PASSED();
}

// ----------------------------------------------------------------------
/*!
 * The actual test suite
 */
// ----------------------------------------------------------------------

class tests : public tframe::tests
{
  virtual const char* error_message_prefix() const { return "\n\t"; }
  void test(void)
  {
    TEST(region_list);
    TEST(time_zone_from_region);
    TEST(time_zone_from_coordinate);
    TEST(time_zone_from_string);
    TEST(zone_name_from_coordinate);
  }
};

}  // namespace TimeZonesTest

//! The main program
int main(void)
{
  using namespace std;
  cout << endl << "TimeZones" << endl << "===============" << endl;
  TimeZonesTest::tests t;
  return t.run();
}

// ======================================================================
