#include "DateTime.h"
#include "DebugTools.h"
#include <boost/test/included/unit_test.hpp>

#include <fstream>
#include <iostream>
#include <vector>
#include <fmt/format.h>

namespace utf = boost::unit_test;
namespace g = boost::gregorian;
namespace pt = boost::posix_time;

utf::test_suite* init_unit_test_suite(int argc, char* argv[])
{
  const char* name = "DateTime tester";
  utf::unit_test_log.set_threshold_level(utf::log_messages);
  utf::framework::master_test_suite().p_name.value = name;
  BOOST_TEST_MESSAGE("");
  BOOST_TEST_MESSAGE(name);
  BOOST_TEST_MESSAGE(std::string(std::strlen(name), '='));
  return NULL;
}

namespace
{
    const std::vector<std::string> test_data =
        {
            "2023-11-1 12:34.45.567890"
            , "1970-01-01 00:00:00.000000"
            , "1950-12-09 11:23:45.123456"
            , "1712-03-23 11:23:45.654321"
        };

}

BOOST_AUTO_TEST_CASE(round_trip_1)
{
    int num_err = 0;
    for (const auto& test_in : test_data) {
        try {
            const pt::ptime src = pt::time_from_string(test_in);
            const Fmi::DateTime t01(src);
            const pt::ptime t02 = t01;
            if (src != t01) {
                std::cout << "    ERROR: input='" << test_in
                          << "', boost::posix_time::ptime:'" << src
                          << "', Fmi::DateTime:'" << t01
                          << "', round_trip:'" << t02
                          << '\'' << std::endl;
                num_err++;
            }
        } catch (const std::exception& e) {
            std::cout << "    ERROR: input='" << test_in << "', got exception '" << e.what()
                      << std::endl;
            num_err++;
        }
    }

    BOOST_REQUIRE_EQUAL(0, num_err);
}

BOOST_AUTO_TEST_CASE(test_to_simple_string)
{
    int num_err = 0;
    for (const auto& test_in : test_data) {
        try {
            const pt::ptime src = pt::time_from_string(test_in);
            const Fmi::DateTime t01(src);
            const std::string s01 = pt::to_simple_string(src);
            const std::string s02 = Fmi::to_simple_string(t01);

            if (s01 != s02) {
                std::cout << "    ERROR: input='" << test_in
                          << "': '" << s01 << "' <=> '" << s02 << '\''
                          << std::endl;
                num_err++;
            }
        } catch (const std::exception& e) {
            std::cout << "    ERROR: input='" << test_in << "', got exception '" << e.what()
                      << std::endl;
            num_err++;
        }
    }

    BOOST_REQUIRE_EQUAL(0, num_err);
}

BOOST_AUTO_TEST_CASE(test_to_iso_string)
{
    int num_err = 0;
    for (const auto& test_in : test_data) {
        try {
            const pt::ptime src = pt::time_from_string(test_in);
            const Fmi::DateTime t01(src);
            const std::string s01 = pt::to_iso_string(src);
            const std::string s02 = Fmi::to_iso_string(t01);

            if (s01 != s02) {
                std::cout << "    ERROR: input='" << test_in
                          << "': '" << s01 << "' <=> '" << s02 << '\''
                          << std::endl;
                num_err++;
            }
        } catch (const std::exception& e) {
            std::cout << "    ERROR: input='" << test_in << "', got exception '" << e.what()
                      << std::endl;
            num_err++;
        }
    }

    BOOST_REQUIRE_EQUAL(0, num_err);
}

BOOST_AUTO_TEST_CASE(test_to_iso_extended_string)
{
    int num_err = 0;
    for (const auto& test_in : test_data) {
        try {
            const pt::ptime src = pt::time_from_string(test_in);
            const Fmi::DateTime t01(src);
            const std::string s01 = pt::to_iso_extended_string(src);
            const std::string s02 = Fmi::to_iso_extended_string(t01);

            if (s01 != s02) {
                std::cout << "    ERROR: input='" << test_in
                          << "': '" << s01 << "' <=> '" << s02 << '\''
                          << std::endl;
                num_err++;
            }
        } catch (const std::exception& e) {
            std::cout << "    ERROR: input='" << test_in << "', got exception '" << e.what()
                      << std::endl;
            num_err++;
        }
    }

    BOOST_REQUIRE_EQUAL(0, num_err);
}
