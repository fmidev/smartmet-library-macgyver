#include "LocalDateTime.h"
#include "DebugTools.h"
#include "Exception.h"
#include <boost/test/included/unit_test.hpp>
#include <fstream>
#include <iostream>
#include <vector>
#include <fmt/format.h>

#include <boost/optional/optional_io.hpp>

namespace utf = boost::unit_test;
namespace g = boost::gregorian;
namespace pt = boost::posix_time;

utf::test_suite* init_unit_test_suite(int argc, char* argv[])
{
  const char* name = "LocalDateTime and TimeZone tester";
  utf::unit_test_log.set_threshold_level(utf::log_messages);
  utf::framework::master_test_suite().p_name.value = name;
  BOOST_TEST_MESSAGE("");
  BOOST_TEST_MESSAGE(name);
  BOOST_TEST_MESSAGE(std::string(std::strlen(name), '='));
  return NULL;
}

BOOST_AUTO_TEST_CASE(uninitialized_tz)
{
    Fmi::TimeZone tz;
    BOOST_REQUIRE_THROW(tz.zone_ptr(), Fmi::Exception);
}

BOOST_AUTO_TEST_CASE(get_tz_1)
{
    const std::string z_name = "UTC";
    Fmi::TimeZone tz(z_name);
    const std::string name = tz->name();
    BOOST_CHECK_EQUAL(name, z_name);
}

BOOST_AUTO_TEST_CASE(get_tz_2)
{
    const std::string z_name = "Europe/Helsinki";
    Fmi::TimeZone tz(z_name);
    const std::string name = tz->name();
    BOOST_CHECK_EQUAL(name, z_name);
}

BOOST_AUTO_TEST_CASE(local_date_time_1)
{
    const std::string src = "2023-Nov-01 12:34.45.567890";
    const std::string utc_str = "2023-Nov-01 10:34.45.567890";
    const std::string t_str2 = "2023-Nov-01 16:19.45.567890";
    pt::ptime ptm = pt::time_from_string(src);
    pt::ptime expected_utc = pt::time_from_string(utc_str);
    const Fmi::DateTime time(ptm);
    const Fmi::TimeZone tz1("Europe/Helsinki");
    const Fmi::LocalDateTime l_time(ptm, tz1);

    const pt::ptime local_time = l_time.local_time();
    const pt::ptime utc_time = l_time.utc_time();
    BOOST_CHECK_EQUAL(ptm, local_time);
    BOOST_CHECK_EQUAL(expected_utc, utc_time);

    const pt::ptime ptm2_exp = pt::time_from_string(t_str2);
    const Fmi::TimeZone tz2("Asia/Kathmandu");
    const Fmi::LocalDateTime l_time2 = l_time.to_tz(tz2);
    const pt::ptime ptm2 = l_time2.local_time();
    BOOST_CHECK_EQUAL(ptm2_exp, ptm2);

    //std::cout << tz2->get_info(l_time2.local_time()) << std::endl;
}
