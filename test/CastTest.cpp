// ======================================================================
/*!
 * \file
 * \brief Regression tests for Cast.h
 */
// ======================================================================

#include <boost/test/included/unit_test.hpp>
#include "Cast.h"

using namespace boost::unit_test;

test_suite* init_unit_test_suite(int argc, char* argv[])
{
  const char* name = "Cast tester";
  unit_test_log.set_threshold_level(log_messages);
  framework::master_test_suite().p_name.value = name;
  BOOST_TEST_MESSAGE("");
  BOOST_TEST_MESSAGE(name);
  BOOST_TEST_MESSAGE(std::string(std::strlen(name), '='));
  return NULL;
}

BOOST_AUTO_TEST_CASE(number_cast)
{
  using Fmi::number_cast;

  BOOST_TEST_MESSAGE("Testing Fmi::number_cast<>()");
  BOOST_CHECK_EQUAL(123, number_cast<int>("123"));
  BOOST_CHECK_CLOSE(123.4, number_cast<double>("123.4"), 1e-10);
  BOOST_CHECK_EQUAL(0, number_cast<int>("0"));
  BOOST_CHECK_THROW(number_cast<int>("0.1234"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(str2int_test)
{
  using Fmi::str2int;

  BOOST_TEST_MESSAGE("Testing Fmi::str2int()");
  BOOST_CHECK_EQUAL(123LL, str2int("123"));
  BOOST_CHECK_EQUAL(-123LL, str2int("-123"));
  BOOST_CHECK_EQUAL(-123LL, str2int(" \t -123  "));
  BOOST_CHECK_EQUAL(12345678987654321LL, str2int("12345678987654321"));
  BOOST_CHECK_EQUAL(123LL, str2int("123", -124, 124));
  BOOST_CHECK_EQUAL(123LL, str2int("123", 0, 123));
  BOOST_CHECK_THROW(str2int("123", 0, 122), std::out_of_range);
  BOOST_CHECK_THROW(str2int("123", 124, 200), std::out_of_range);
  BOOST_CHECK_EQUAL(123LL, str2int("123", 123, 200));
  BOOST_CHECK_THROW(str2int("foo"), std::invalid_argument);
  BOOST_CHECK_THROW(str2int("123.4"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(str2uint_test)
{
  using Fmi::str2uint;

  BOOST_TEST_MESSAGE("Testing Fmi::str2uint()");
  BOOST_CHECK_EQUAL(123ULL, str2uint("123"));
  BOOST_CHECK_THROW(str2uint("-123"), std::invalid_argument);
  BOOST_CHECK_EQUAL(123ULL, str2uint(" \t 123  "));
  BOOST_CHECK_EQUAL(123ULL, str2uint(" \t +123  "));
  BOOST_CHECK_EQUAL(12345678987654321ULL, str2uint("12345678987654321"));
  BOOST_CHECK_EQUAL(123ULL, str2uint("123", 0, 124));
  BOOST_CHECK_EQUAL(123ULL, str2uint("123", 0, 123));
  BOOST_CHECK_THROW(str2uint("123", 0, 122), std::out_of_range);
  BOOST_CHECK_THROW(str2uint("123", 124, 200), std::out_of_range);
  BOOST_CHECK_EQUAL(123ULL, str2uint("123", 123, 200));
  BOOST_CHECK_THROW(str2uint("foo"), std::invalid_argument);
  BOOST_CHECK_THROW(str2uint("123.4"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(strdouble_test)
{
  using Fmi::str2double;

  BOOST_TEST_MESSAGE("Testing Fmi::str2double()");
  BOOST_CHECK_CLOSE(123.0, str2double("123"), 1e-10);
  BOOST_CHECK_CLOSE(-123.4, str2double("-123.4"), 1e-10);
  BOOST_CHECK_CLOSE(-123.4, str2double(" \t -123.4 "), 1e-10);
  BOOST_CHECK_CLOSE(-123.4, str2double("-123.4", -200.0, 200.0), 1e-10);
  BOOST_CHECK_THROW(str2double("-123", -100.0, 100.0), std::out_of_range);
  BOOST_CHECK_THROW(str2double("foo"), std::invalid_argument);
  BOOST_CHECK_THROW(str2double("123.4.5"), std::invalid_argument);
  BOOST_CHECK_THROW(str2double("123.4    .5"), std::invalid_argument);
}

// ======================================================================
