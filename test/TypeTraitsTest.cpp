#include "TypeTraits.h"
#include <iostream>
#include <boost/test/included/unit_test.hpp>

using namespace boost::unit_test;

test_suite* init_unit_test_suite(int argc, char* argv[])
{
  const char* name = "TypeTraits tester";
  // unit_test_log.set_threshold_level(log_test_units);
  unit_test_log.set_threshold_level(log_messages);
  framework::master_test_suite().p_name.value = name;
  BOOST_TEST_MESSAGE("");
  BOOST_TEST_MESSAGE(name);
  BOOST_TEST_MESSAGE(std::string(std::strlen(name), '='));
  return nullptr;
}

BOOST_AUTO_TEST_CASE(test_are_all_convertible_to)
{
    int i1 = 42;
    short int s1 = 7;
    const char * example1 = "Some text 1";
    const std::string example2 = "Some text 2";

    BOOST_CHECK_EQUAL((Fmi::are_convertible_to<double, int, short int, double>()), true);
    BOOST_CHECK_EQUAL((Fmi::are_convertible_to<std::string, const char *, const std::string>()), true);
    BOOST_CHECK_EQUAL((Fmi::are_convertible_to<double>(i1, s1, 3.14)), true);
    BOOST_CHECK_EQUAL((Fmi::are_convertible_to<std::string, const char *, const std::string>(example1, example2)), true);
    BOOST_CHECK_EQUAL((Fmi::are_convertible_to<int>(i1, example1, 3)), false);
    BOOST_CHECK_EQUAL((Fmi::are_convertible_to<int>(i1, example2, 3)), false);
}
