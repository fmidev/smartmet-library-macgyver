// ======================================================================
/*!
 * \file
 * \brief Regression tests for date_time::LocalDateTime
 * */
// ======================================================================

#include "date_time/LocalDateTime.h"
#include "date_time/TimeZonePtr.h"
#include "Exception.h"
#include "DebugTools.h"
#include <boost/test/included/unit_test.hpp>
#include <algorithm>

#include <boost/date_time/local_time/local_time.hpp>

using namespace boost::unit_test;
using namespace std::string_literals;
namespace g = boost::gregorian;

#define DEBUG(x) SHOW_EXCEPTIONS(x)
//#define DEBUG(x) x

static const char* default_regions = "/usr/share/smartmet/timezones/date_time_zonespec.csv";

namespace g = boost::gregorian;
namespace pt = boost::posix_time;
namespace lt = boost::local_time;

namespace
{
    boost::local_time::tz_database itsRegions;

    lt::time_zone_ptr get_boost_tz(const std::string& name)
    {
        auto ptr = itsRegions.time_zone_from_region(name);
        if (!ptr)
            throw Fmi::Exception(BCP, "Unknown timezone").addParameter("Name", name);
        return ptr;
    }

    lt::local_date_time get_boost_ldt(const Fmi::date_time::LocalDateTime& ldt)
    {
        const int year = ldt.date().year();
        const int month = ldt.date().month();
        const int day = ldt.date().day();
        const int hours = ldt.time_of_day().hours();
        const int minutes = ldt.time_of_day().minutes();
        const int seconds = ldt.time_of_day().seconds();
        const int fraction = ldt.time_of_day().fractional_seconds();

        const g::date d(year, month, day);
        const pt::time_duration td(hours, minutes, seconds, fraction);
        const lt::time_zone_ptr tz = get_boost_tz(ldt.zone()->name());
        return lt::local_date_time(d, td, tz, lt::local_date_time::NOT_DATE_TIME_ON_ERROR);
    }

    std::string as_string(const lt::local_date_time& ldt)
    {
        std::ostringstream os;
        os << ldt;
        return os.str();
    }
}

test_suite* init_unit_test_suite(int argc, char* argv[])
{
  const char* name = "Fmi::date_time::LocalDateTime tester";
  // unit_test_log.set_threshold_level(log_test_units);
  unit_test_log.set_threshold_level(log_messages);
  framework::master_test_suite().p_name.value = name;
  BOOST_TEST_MESSAGE("");
  BOOST_TEST_MESSAGE(name);
  BOOST_TEST_MESSAGE(std::string(std::strlen(name), '='));
  itsRegions.load_from_file(default_regions);
  std::setlocale(LC_ALL, "C");
  return NULL;
}

BOOST_AUTO_TEST_CASE(construct_and_extract_1)
{
    using namespace Fmi::date_time;
    BOOST_TEST_MESSAGE("Fmi::date_time::LocalDateTime: construction and extraction (1)");

    TimeZonePtr tz1("Europe/Helsinki");
    TimeZonePtr tz2("Etc/UTC");
    DateTime dtm1 = time_from_string("2014-02-01 12:34:56.789");
    DateTime dtm2 = time_from_string("2014-05-01 12:34:56.789");
    LocalDateTime ldt1(dtm1, tz1);

    BOOST_CHECK_EQUAL(ldt1.date().year(), 2014);
    BOOST_CHECK_EQUAL(ldt1.date().month(), 2);
    BOOST_CHECK_EQUAL(ldt1.date().day(), 1);
    BOOST_CHECK_EQUAL(ldt1.time_of_day().hours(), 12);
    BOOST_CHECK_EQUAL(ldt1.time_of_day().minutes(), 34);
    BOOST_CHECK_EQUAL(ldt1.time_of_day().seconds(), 56);
    BOOST_CHECK_EQUAL(ldt1.time_of_day().fractional_seconds(), 789000);

    BOOST_CHECK(!ldt1.dst_on());
    BOOST_CHECK_EQUAL(ldt1.offset(), Hours(2));

    BOOST_CHECK_EQUAL(ldt1.utc_time(), dtm1 - Fmi::date_time::hours(2));

    //std::cout << ldt1.get_sys_info() << std::endl;

    LocalDateTime ldt2(dtm2, tz1);

    BOOST_CHECK_EQUAL(ldt2.date().year(), 2014);
    BOOST_CHECK_EQUAL(ldt2.date().month(), 5);
    BOOST_CHECK_EQUAL(ldt2.date().day(), 1);
    BOOST_CHECK_EQUAL(ldt2.time_of_day().hours(), 12);
    BOOST_CHECK_EQUAL(ldt2.time_of_day().minutes(), 34);
    BOOST_CHECK_EQUAL(ldt2.time_of_day().seconds(), 56);
    BOOST_CHECK_EQUAL(ldt2.time_of_day().fractional_seconds(), 789000);

    BOOST_CHECK(ldt2.dst_on());
    BOOST_CHECK_EQUAL(ldt2.offset(), Hours(3));
    BOOST_CHECK_EQUAL(ldt2.utc_time(), dtm2 - Fmi::date_time::hours(3));

    //std::cout << ldt2.get_sys_info() << std::endl;

    LocalDateTime ldt3(dtm2, tz2);

    BOOST_CHECK_EQUAL(ldt3.date().year(), 2014);
    BOOST_CHECK_EQUAL(ldt3.date().month(), 5);
    BOOST_CHECK_EQUAL(ldt3.date().day(), 1);
    BOOST_CHECK_EQUAL(ldt3.time_of_day().hours(), 12);
    BOOST_CHECK_EQUAL(ldt3.time_of_day().minutes(), 34);
    BOOST_CHECK_EQUAL(ldt3.time_of_day().seconds(), 56);
    BOOST_CHECK_EQUAL(ldt3.time_of_day().fractional_seconds(), 789000);

    BOOST_CHECK(!ldt3.dst_on());
    BOOST_CHECK_EQUAL(ldt3.offset(), Hours(0));
    BOOST_CHECK_EQUAL(ldt3.utc_time(), ldt3.local_time());

    //std::cout << ldt3.get_sys_info() << std::endl;
}

BOOST_AUTO_TEST_CASE(dst_test_1)
{
    BOOST_TEST_MESSAGE("Fmi::date_time::LocalDateTime: handling beging of DST");
    Fmi::date_time::TimeZonePtr tz("Europe/Helsinki");

    Fmi::date_time::Date date(2023, 3, 26);
    Fmi::date_time::TimeDuration time(2, 59, 59);
    Fmi::date_time::DateTime dt1(date, time);                     // 02:59:59
    Fmi::date_time::DateTime dt2 = DEBUG(dt1 + Fmi::date_time::seconds(2));         // 03:00:01
    Fmi::date_time::DateTime dt3 = DEBUG(dt2 + Fmi::date_time::hours(1));           // 04:00:01

    // 1 second before begin of summertime
    Fmi::date_time::LocalDateTime ldt1(dt1, tz);
    BOOST_CHECK_EQUAL(ldt1.is_special(), false);
    BOOST_CHECK_EQUAL(ldt1.local_time(), dt1);
    BOOST_CHECK_EQUAL(ldt1.dst_on(), false);

    // 1 second in gap (begin of summertime) -> invalid local time
    Fmi::date_time::LocalDateTime ldt2(dt2, tz);
    BOOST_CHECK_EQUAL(ldt2.is_special(), true);

    Fmi::date_time::LocalDateTime ldt3(dt3, tz);
    BOOST_CHECK_EQUAL(ldt3.is_special(), false);
    BOOST_CHECK_EQUAL(ldt3.local_time(), dt3);
    BOOST_CHECK_EQUAL(ldt3.dst_on(), true);
}

BOOST_AUTO_TEST_CASE(dst_test_2)
{
    BOOST_TEST_MESSAGE("Fmi::date_time::LocalDateTime: handling end of DST");
    Fmi::date_time::TimeZonePtr tz("Europe/Helsinki");

    Fmi::date_time::Date date(2023, 10, 29);
    Fmi::date_time::TimeDuration time(2, 59, 59);
    Fmi::date_time::DateTime dt1(date, time);                     // 02:59:59
    Fmi::date_time::DateTime dt2 = dt1 + Fmi::date_time::seconds(2);         // 03:00:01
    Fmi::date_time::DateTime dt3 = dt2 + Fmi::date_time::hours(1);           // 04:00:01

    // 1 second before end of summertime
    Fmi::date_time::LocalDateTime ldt1(dt1, tz);
    Fmi::date_time::DateTime dt1_utc = ldt1.utc_time();
    BOOST_CHECK_EQUAL(dt1_utc, dt1 - Fmi::date_time::hours(3));
    Fmi::date_time::LocalDateTime ldt1_b = ldt1.to_tz(tz);
    BOOST_CHECK_EQUAL(ldt1_b, ldt1);

    Fmi::date_time::LocalDateTime ldt3(dt3, tz);
    Fmi::date_time::DateTime dt3_utc = ldt3.utc_time();
    BOOST_CHECK_EQUAL(dt3_utc, dt3 - Fmi::date_time::hours(2));
    Fmi::date_time::LocalDateTime ldt3_b = ldt3.to_tz(tz);
    BOOST_CHECK_EQUAL(ldt3_b, ldt3);
}

BOOST_AUTO_TEST_CASE(tz_convert_1)
{
  BOOST_TEST_MESSAGE("Fmi::date_time::LocalDateTime: conversion between timezones");
  Fmi::date_time::TimeZonePtr tz1("Europe/Helsinki");
  Fmi::date_time::TimeZonePtr tz2("Europe/Riga");
  Fmi::date_time::TimeZonePtr tz3("UTC");
  Fmi::date_time::TimeZonePtr tz4("Asia/Kathmandu");

  Fmi::date_time::DateTime dt1(Fmi::date_time::Date(1961, 12, 31), Fmi::date_time::TimeDuration(22, 30, 00));
  Fmi::date_time::DateTime dt2(Fmi::date_time::Date(1961, 12, 31), Fmi::date_time::TimeDuration(23, 30, 00));
  Fmi::date_time::DateTime dt3(Fmi::date_time::Date(1961, 12, 31), Fmi::date_time::TimeDuration(20, 30, 00));
  Fmi::date_time::DateTime dt4(Fmi::date_time::Date(1962,  1,  1), Fmi::date_time::TimeDuration( 2, 00, 00));

  Fmi::date_time::LocalDateTime ldt2(dt2, tz2);
  Fmi::date_time::LocalDateTime ldt1 = ldt2.to_tz(tz1);
  Fmi::date_time::LocalDateTime ldt3 = ldt2.to_tz(tz3);
  Fmi::date_time::LocalDateTime ldt4 = ldt2.to_tz(tz4);

  BOOST_CHECK_EQUAL(ldt2.is_special(), false);

  BOOST_CHECK_EQUAL(Fmi::date_time::to_iso_extended_string(ldt1), "1961-12-31T22:30:00+02:00"s);
  BOOST_CHECK_EQUAL(Fmi::date_time::to_iso_extended_string(ldt2), "1961-12-31T23:30:00+03:00"s);
  BOOST_CHECK_EQUAL(Fmi::date_time::to_iso_extended_string(ldt3), "1961-12-31T20:30:00Z"s);
  BOOST_CHECK_EQUAL(Fmi::date_time::to_iso_extended_string(ldt4), "1962-01-01T02:00:00+05:30"s);
}

BOOST_AUTO_TEST_CASE(advance_1)
{
  Fmi::date_time::TimeZonePtr tz("Europe/Helsinki");

  Fmi::date_time::Date date(2023, 3, 26);
  Fmi::date_time::TimeDuration time(2, 59, 59);
  Fmi::date_time::DateTime dt1(date, time);                     // 02:59:59
  Fmi::date_time::LocalDateTime ldt0(dt1, tz);

  Fmi::date_time::LocalDateTime ldt1(ldt0);
  ldt1.advance(Fmi::date_time::seconds(2));

  // Jump over gap due to DST begin
  BOOST_CHECK_EQUAL(ldt1.is_special(), false);
  BOOST_CHECK_EQUAL(ldt1.local_time(), dt1 + Fmi::date_time::seconds(3602));

  Fmi::date_time::TimeDuration diff = ldt1 - ldt0;
  BOOST_CHECK_EQUAL(diff, Fmi::date_time::seconds(2));

  ldt1.advance(-Fmi::date_time::seconds(3));
  BOOST_CHECK_EQUAL(ldt1.is_special(), false);
  BOOST_CHECK_EQUAL(ldt1.local_time(), dt1 - Fmi::date_time::seconds(1));
}

BOOST_AUTO_TEST_CASE(conversion_to_boost_1)
{
    using namespace Fmi::date_time;

    TimeZonePtr tz1("Europe/Helsinki");
    lt::time_zone_ptr btz = get_boost_tz(tz1->name());

    DateTime dtm1 = time_from_string("2014-02-01 12:34:56.789");
    DateTime dtm2 = time_from_string("2014-02-01 12:34:56");
    LocalDateTime ldt1(dtm1, tz1);
    LocalDateTime ldt2(dtm2, tz1);

    lt::local_date_time b_ldt1 = get_boost_ldt(ldt1);
    lt::local_date_time b_ldt2 = get_boost_ldt(ldt2);

    BOOST_CHECK_EQUAL(b_ldt1.date().year(), 2014);
    BOOST_CHECK_EQUAL(b_ldt1.date().month(), 2);
    BOOST_CHECK_EQUAL(b_ldt1.date().day(), 1);
    const long mks1 = ldt1.local_time().time_of_day().total_microseconds();
    const long bmks1 = b_ldt1.local_time().time_of_day().total_microseconds();
    BOOST_CHECK_EQUAL(mks1, bmks1);

    BOOST_CHECK_EQUAL(as_string(b_ldt1), ldt1.as_simple_string());

    BOOST_CHECK_EQUAL(b_ldt2.date().year(), 2014);
    BOOST_CHECK_EQUAL(b_ldt2.date().month(), 2);
    BOOST_CHECK_EQUAL(b_ldt2.date().day(), 1);
    const long mks2 = ldt2.local_time().time_of_day().total_microseconds();
    const long bmks2 = b_ldt2.local_time().time_of_day().total_microseconds();
    BOOST_CHECK_EQUAL(mks2, bmks2);
}


BOOST_AUTO_TEST_CASE(advance_comparision_with_boost_1)
{
    using namespace Fmi::date_time;

    BOOST_TEST_MESSAGE("Fmi::date_time::DateTime: advance and comparision with boost (1)");

    const auto s_dt1 = "2024-Jan-01 00:00:01"s;
    const auto s_dt2 = "2025-Jan-01 00:00:01"s;
    const auto s_inc = "00:37:34.654789"s;

    TimeZonePtr tz1("Europe/Helsinki");

    const auto inc1 = duration_from_string(s_inc);
    const auto b_inc = boost::posix_time::duration_from_string(s_inc);
 
    DateTime dt1 = time_from_string(s_dt1);
 
    LocalDateTime ldt1(time_from_string(s_dt1), tz1);
    LocalDateTime ldt2(time_from_string(s_dt2), tz1);
    LocalDateTime ldt_start = ldt1;

    auto b_ldt1 = get_boost_ldt(ldt1);

    while (ldt1 < ldt2)
    {
        ldt1.advance(inc1);
        b_ldt1 += b_inc;
        const std::string s1 = ldt1.local_time().as_string();
        const std::string s2 = pt::to_simple_string(b_ldt1.local_time());
        //std::cout << s1 << " == " << s2 << std::endl;
        BOOST_REQUIRE_EQUAL(s1, s2);
    }  

    while (ldt1 > ldt_start)
    {
        ldt1.advance(-inc1);
        b_ldt1 -= b_inc;
        const std::string s1 = ldt1.local_time().as_string();
        const std::string s2 = pt::to_simple_string(b_ldt1.local_time());
        //std::cout << s1 << " == " << s2 << std::endl;
        BOOST_REQUIRE_EQUAL(s1, s2);
    }
}

BOOST_AUTO_TEST_CASE(test_make_date)
{
    using namespace Fmi::date_time;

    BOOST_TEST_MESSAGE("Fmi::date_time::DateTime: make_date");

    Fmi::date_time::Date date(2024, 10, 27);
    TimeZonePtr tz1("Europe/Helsinki");

    int num_err = 0;
    for (int i = 0; i < 33; i++)
    {
        Fmi::date_time::LocalDateTime ldt1;
        const auto time = Fmi::date_time::Minutes(10*i);
        ldt1 = make_time(date, time, tz1);
        const std::string exp_abbrev = time.hours() < 4 ? "EEST" : "EET";
        if (ldt1.abbrev() != exp_abbrev)
        {
            if (num_err < 10)
            {
                std::cout << "Error: " << ldt1.abbrev() << " != " << exp_abbrev
                          << " at " << Fmi::date_time::to_iso_extended_string(ldt1)
                          << std::endl;
            }
            num_err++;
        }
    }
    BOOST_REQUIRE_EQUAL(num_err, 0);
}
