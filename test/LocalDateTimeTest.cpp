// ======================================================================
/*!
 * \file
 * \brief Regression tests for LocalDateTime
 */
// ======================================================================

#include "LocalDateTime.h"
#include "Exception.h"
#include <boost/test/included/unit_test.hpp>
#include <algorithm>

#include <iostream>

using namespace boost::unit_test;
using namespace std::string_literals;

test_suite* init_unit_test_suite(int argc, char* argv[])
{
  const char* name = "LocalDateTime tester";
  // unit_test_log.set_threshold_level(log_test_units);
  unit_test_log.set_threshold_level(log_messages);
  framework::master_test_suite().p_name.value = name;
  BOOST_TEST_MESSAGE("");
  BOOST_TEST_MESSAGE(name);
  BOOST_TEST_MESSAGE(std::string(std::strlen(name), '='));
  return NULL;
}

BOOST_AUTO_TEST_CASE(dst_test_1)
{
    Fmi::TimeZonePtr tz("Europe/Helsinki");

    Fmi::Date date(2023, 3, 26);
    Fmi::TimeDuration time(2, 59, 59);
    Fmi::DateTime dt1(date, time);                     // 02:59:59
    Fmi::DateTime dt2 = dt1 + Fmi::Seconds(2);         // 03:00:01
    Fmi::DateTime dt3 = dt2 + Fmi::Hours(1);           // 04:00:01

    // 1 second before begin of summertime
    Fmi::LocalDateTime ldt1(dt1, tz);
    BOOST_CHECK_EQUAL(ldt1.is_special(), false);
    BOOST_CHECK_EQUAL(ldt1.local_time(), dt1);
    BOOST_CHECK_EQUAL(ldt1.dst_on(), false);

    // 1 second in gap (begin of summertime) -> invalid local time
    Fmi::LocalDateTime ldt2(dt2, tz);
    BOOST_CHECK_EQUAL(ldt2.is_special(), true);

    Fmi::LocalDateTime ldt3(dt3, tz);
    BOOST_CHECK_EQUAL(ldt3.is_special(), false);
    BOOST_CHECK_EQUAL(ldt3.local_time(), dt3);
    BOOST_CHECK_EQUAL(ldt3.dst_on(), true);
}

BOOST_AUTO_TEST_CASE(dst_test_2)
{
    Fmi::TimeZonePtr tz("Europe/Helsinki");

    Fmi::Date date(2023, 10, 29);
    Fmi::TimeDuration time(2, 59, 59);
    Fmi::DateTime dt1(date, time);                     // 02:59:59
    Fmi::DateTime dt2 = dt1 + Fmi::Seconds(2);         // 03:00:01
    Fmi::DateTime dt3 = dt2 + Fmi::Hours(1);           // 04:00:01

    // 1 second before end of summertime
    Fmi::LocalDateTime ldt1(dt1, tz);
    Fmi::DateTime dt1_utc = ldt1.utc_time();
    BOOST_CHECK_EQUAL(dt1_utc, dt1 - Fmi::Hours(3));
    Fmi::LocalDateTime ldt1_b = ldt1.to_tz(tz);
    BOOST_CHECK_EQUAL(ldt1_b, ldt1);

    Fmi::LocalDateTime ldt3(dt3, tz);
    Fmi::DateTime dt3_utc = ldt3.utc_time();
    BOOST_CHECK_EQUAL(dt3_utc, dt3 - Fmi::Hours(2));
    Fmi::LocalDateTime ldt3_b = ldt3.to_tz(tz);
    BOOST_CHECK_EQUAL(ldt3_b, ldt3);
}

BOOST_AUTO_TEST_CASE(tz_convert_1)
{
  Fmi::TimeZonePtr tz1("Europe/Helsinki");
  Fmi::TimeZonePtr tz2("Europe/Riga");
  Fmi::TimeZonePtr tz3("UTC");
  Fmi::TimeZonePtr tz4("Asia/Kathmandu");

  Fmi::DateTime dt1(Fmi::Date(1961, 12, 31), Fmi::TimeDuration(22, 30, 00));
  Fmi::DateTime dt2(Fmi::Date(1961, 12, 31), Fmi::TimeDuration(23, 30, 00));
  Fmi::DateTime dt3(Fmi::Date(1961, 12, 31), Fmi::TimeDuration(20, 30, 00));
  Fmi::DateTime dt4(Fmi::Date(1962,  1,  1), Fmi::TimeDuration( 2, 00, 00));

  Fmi::LocalDateTime ldt2(dt2, tz2);
  Fmi::LocalDateTime ldt1 = ldt2.to_tz(tz1);
  Fmi::LocalDateTime ldt3 = ldt2.to_tz(tz3);
  Fmi::LocalDateTime ldt4 = ldt2.to_tz(tz4);

  BOOST_CHECK_EQUAL(ldt2.is_special(), false);
  BOOST_CHECK_EQUAL(Fmi::to_iso_extended_string(ldt1), "1961-12-31T22:30:00+02:00"s);
  BOOST_CHECK_EQUAL(Fmi::to_iso_extended_string(ldt2), "1961-12-31T23:30:00+03:00"s);
  BOOST_CHECK_EQUAL(Fmi::to_iso_extended_string(ldt3), "1961-12-31T20:30:00Z"s);
  BOOST_CHECK_EQUAL(Fmi::to_iso_extended_string(ldt4), "1962-01-01T02:00:00+05:30"s);
}

BOOST_AUTO_TEST_CASE(advance_1)
{
  Fmi::TimeZonePtr tz("Europe/Helsinki");

  Fmi::Date date(2023, 3, 26);
  Fmi::TimeDuration time(2, 59, 59);
  Fmi::DateTime dt1(date, time);                     // 02:59:59
  Fmi::LocalDateTime ldt0(dt1, tz);

  Fmi::LocalDateTime ldt1(ldt0);
  ldt1.advance(Fmi::Seconds(2));

  // Jump over gap due to DST begin
  BOOST_CHECK_EQUAL(ldt1.is_special(), false);
  BOOST_CHECK_EQUAL(ldt1.local_time(), dt1 + Fmi::Seconds(3602));

  Fmi::TimeDuration diff = ldt1 - ldt0;
  BOOST_CHECK_EQUAL(diff, Fmi::Seconds(2));

  ldt1.advance(-Fmi::Seconds(3));
  BOOST_CHECK_EQUAL(ldt1.is_special(), false);
  BOOST_CHECK_EQUAL(ldt1.local_time(), dt1 - Fmi::Seconds(1));
}
