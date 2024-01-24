// ======================================================================
/*!
 * \file
 * \brief Regression tests for date_time::DateTime
 * */
// ======================================================================

#include "date_time/DateTime.h"
#include "Exception.h"
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/test/included/unit_test.hpp>
#include <algorithm>

using namespace boost::unit_test;
namespace g = boost::gregorian;

test_suite* init_unit_test_suite(int argc, char* argv[])
{
  const char* name = "Fmi::date_time::DateTime tester";
  // unit_test_log.set_threshold_level(log_test_units);
  unit_test_log.set_threshold_level(log_messages);
  framework::master_test_suite().p_name.value = name;
  BOOST_TEST_MESSAGE("");
  BOOST_TEST_MESSAGE(name);
  BOOST_TEST_MESSAGE(std::string(std::strlen(name), '='));
  return NULL;
}

BOOST_AUTO_TEST_CASE(construct_and_extract_test_1)
{
    BOOST_TEST_MESSAGE("Fmi::date_time::DateTime: construction and extraction (1)");
    Fmi::date_time::TimeDuration td1(12, 34, 45, 123456);
    Fmi::date_time::Date d1(2014, 1, 24);
    Fmi::date_time::DateTime dt1(d1, td1);
    BOOST_CHECK_EQUAL(dt1.date(), d1);
    BOOST_CHECK_EQUAL(dt1.time_of_day(), td1);
}

BOOST_AUTO_TEST_CASE(construct_and_extract_test_2)
{
    BOOST_TEST_MESSAGE("Fmi::date_time::DateTime: construction and extraction (2)");
    Fmi::date_time::TimeDuration td1(12, 34, 45, 123456);
    Fmi::date_time::Date d1(1723, 1, 24);
    Fmi::date_time::DateTime dt1(d1, td1);
    Fmi::date_time::Date d2 = dt1.date();
    Fmi::date_time::TimeDuration td2 = dt1.time_of_day();
    BOOST_CHECK_EQUAL(d1, d2);
    BOOST_CHECK_EQUAL(td1, td2);
}

BOOST_AUTO_TEST_CASE(to_simple_string_test_1)
{
    BOOST_TEST_MESSAGE("Fmi::date_time::DateTime: to_simple_string");
    Fmi::date_time::TimeDuration td1(12, 34, 45);
    Fmi::date_time::Date d1(2024, 1, 24);
    Fmi::date_time::DateTime dt1(d1, td1);
    std::string str = Fmi::date_time::to_simple_string(dt1);
    BOOST_CHECK_EQUAL(str, "2024-Jan-24 12:34:45");

    dt1 += Fmi::date_time::microseconds(123456);
    str = Fmi::date_time::to_simple_string(dt1);
    BOOST_CHECK_EQUAL(str, "2024-Jan-24 12:34:45.123456");
}

BOOST_AUTO_TEST_CASE(to_iso_string_test_1)
{
    BOOST_TEST_MESSAGE("Fmi::date_time::DateTime: to_iso_string");
    Fmi::date_time::TimeDuration td1(12, 34, 45);
    Fmi::date_time::Date d1(2024, 1, 24);
    Fmi::date_time::DateTime dt1(d1, td1);
    std::string str = Fmi::date_time::to_iso_string(dt1);
    BOOST_CHECK_EQUAL(str, "20240124T123445");

    dt1 += Fmi::date_time::microseconds(123456);
    str = Fmi::date_time::to_iso_string(dt1);
    BOOST_CHECK_EQUAL(str, "20240124T123445.123456");
}

BOOST_AUTO_TEST_CASE(to_iso_extended_string_test_1)
{
    BOOST_TEST_MESSAGE("Fmi::date_time::DateTime: to_iso_extended_string");
    Fmi::date_time::TimeDuration td1(12, 34, 45);
    Fmi::date_time::Date d1(2024, 1, 24);
    Fmi::date_time::DateTime dt1(d1, td1);
    std::string str = Fmi::date_time::to_iso_extended_string(dt1);
    BOOST_CHECK_EQUAL(str, "2024-01-24T12:34:45");

    dt1 += Fmi::date_time::microseconds(123456);
    str = Fmi::date_time::to_iso_extended_string(dt1);
    BOOST_CHECK_EQUAL(str, "2024-01-24T12:34:45.123456");
}
