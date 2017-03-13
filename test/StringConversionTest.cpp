// ======================================================================
/*!
 * \file
 * \brief Regression tests for StringConversion.h
 */
// ======================================================================

#include <boost/test/included/unit_test.hpp>
#include "StringConversion.h"
#include <boost/date_time/posix_time/posix_time_types.hpp>

using namespace boost::unit_test;

test_suite* init_unit_test_suite(int argc, char* argv[])
{
  const char* name = "String conversions tester";
  unit_test_log.set_threshold_level(log_messages);
  framework::master_test_suite().p_name.value = name;
  BOOST_TEST_MESSAGE("");
  BOOST_TEST_MESSAGE(name);
  BOOST_TEST_MESSAGE(std::string(std::strlen(name), '='));
  return NULL;
}

BOOST_AUTO_TEST_CASE(stoi)
{
  BOOST_TEST_MESSAGE(" + Fmi::stoi()");
  BOOST_CHECK_EQUAL(0, Fmi::stoi("0"));
  BOOST_CHECK_EQUAL(0, Fmi::stoi("+0"));
  BOOST_CHECK_EQUAL(0, Fmi::stoi("-0"));
  BOOST_CHECK_EQUAL(123, Fmi::stoi("123"));
  BOOST_CHECK_EQUAL(-123, Fmi::stoi("-123"));

  BOOST_CHECK_THROW(Fmi::stoi("321.1234"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stoi("123456789012"), std::bad_cast);
  BOOST_CHECK_THROW(Fmi::stoi("ABC"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stoi("12A"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stoi("12A"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stoi("12 "), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stoi(" 12"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stoi(" 12 "), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(stol)
{
  BOOST_TEST_MESSAGE(" + Fmi::stol()");
  BOOST_CHECK_EQUAL(0L, Fmi::stol("0"));
  BOOST_CHECK_EQUAL(0L, Fmi::stol("+0"));
  BOOST_CHECK_EQUAL(0L, Fmi::stol("-0"));
  BOOST_CHECK_EQUAL(123L, Fmi::stol("123"));
  BOOST_CHECK_EQUAL(-123L, Fmi::stol("-123"));
  BOOST_CHECK_EQUAL(123456789012, Fmi::stol("123456789012"));

  BOOST_CHECK_THROW(Fmi::stol("321.1234"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stol("ABC"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stol("12A"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stol("12A"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stol("12 "), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stol(" 12"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stol(" 12 "), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(stoul)
{
  BOOST_TEST_MESSAGE(" + Fmi::stoul()");
  BOOST_CHECK_EQUAL(0UL, Fmi::stoul("0"));
  BOOST_CHECK_EQUAL(123UL, Fmi::stoul("123"));
  BOOST_CHECK_EQUAL(123456789012, Fmi::stoul("123456789012"));

  BOOST_CHECK_THROW(Fmi::stoul("+0"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stoul("-0"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stoul("-123"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stoul("321.1234"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stoul("ABC"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stoul("12A"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stoul("12A"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stoul("12 "), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stoul(" 12"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stoul(" 12 "), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(stof)
{
  BOOST_TEST_MESSAGE(" + Fmi::stof()");
  BOOST_CHECK_EQUAL(0.f, Fmi::stof("0"));
  BOOST_CHECK_EQUAL(0.f, Fmi::stof("+0"));
  BOOST_CHECK_EQUAL(0.f, Fmi::stof("-0"));
  BOOST_CHECK_EQUAL(123.f, Fmi::stof("123"));
  BOOST_CHECK_EQUAL(-123.f, Fmi::stof("-123"));
  BOOST_CHECK_CLOSE(321.1234f, Fmi::stof("321.1234"), 0.001);
  BOOST_CHECK_CLOSE(123456789012.f, Fmi::stof("123456789012"), 1);
  BOOST_CHECK_THROW(Fmi::stof("ABC"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stof("12A"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stof("12A"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stof("12 "), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stof(" 12"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stof(" 12 "), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stof("NaN"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stof("NAN"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stof("INF"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stof("+INF"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stof("-INF"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(stod)
{
  BOOST_TEST_MESSAGE(" + Fmi::stod()");
  BOOST_CHECK_EQUAL(0, Fmi::stod("0"));
  BOOST_CHECK_EQUAL(0, Fmi::stod("+0"));
  BOOST_CHECK_EQUAL(0, Fmi::stod("-0"));
  BOOST_CHECK_EQUAL(123, Fmi::stod("123"));
  BOOST_CHECK_EQUAL(-123, Fmi::stod("-123"));
  BOOST_CHECK_CLOSE(321.1234f, Fmi::stod("321.1234"), 0.001);
  BOOST_CHECK_CLOSE(123456789012, Fmi::stod("123456789012"), 1);
  BOOST_CHECK_THROW(Fmi::stod("ABC"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stod("12A"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stod("12A"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stod("12 "), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stod(" 12"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stod(" 12 "), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stod("NaN"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stod("NAN"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stod("INF"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stod("+INF"), std::invalid_argument);
  BOOST_CHECK_THROW(Fmi::stod("-INF"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(to_string)
{
  BOOST_TEST_MESSAGE(" + Fmi::to_string()");
  BOOST_CHECK_EQUAL("0", Fmi::to_string(0));
  BOOST_CHECK_EQUAL("1", Fmi::to_string(1));
  BOOST_CHECK_EQUAL("-1", Fmi::to_string(-1));
  BOOST_CHECK_EQUAL("1234567890", Fmi::to_string(1234567890));
  BOOST_CHECK_EQUAL("1234567890123456789", Fmi::to_string(1234567890123456789));
  BOOST_CHECK_EQUAL("-1234567890", Fmi::to_string(-1234567890));
  BOOST_CHECK_EQUAL("-1234567890123456789", Fmi::to_string(-1234567890123456789));

  BOOST_CHECK_EQUAL("0", Fmi::to_string(0L));
  BOOST_CHECK_EQUAL("1", Fmi::to_string(1L));
  BOOST_CHECK_EQUAL("-1", Fmi::to_string(-1L));
  BOOST_CHECK_EQUAL("1234567890", Fmi::to_string(1234567890L));
  BOOST_CHECK_EQUAL("1234567890123456789", Fmi::to_string(1234567890123456789L));
  BOOST_CHECK_EQUAL("-1234567890", Fmi::to_string(-1234567890L));
  BOOST_CHECK_EQUAL("-1234567890123456789", Fmi::to_string(-1234567890123456789L));

  BOOST_CHECK_EQUAL("0", Fmi::to_string(0UL));
  BOOST_CHECK_EQUAL("1", Fmi::to_string(1UL));
  BOOST_CHECK_EQUAL("1234567890", Fmi::to_string(1234567890UL));
  BOOST_CHECK_EQUAL("1234567890123456789", Fmi::to_string(1234567890123456789UL));

  BOOST_CHECK_EQUAL("0", Fmi::to_string(0.0));
  BOOST_CHECK_EQUAL("0.1", Fmi::to_string(0.1));
  BOOST_CHECK_EQUAL("1", Fmi::to_string(1.0));
  BOOST_CHECK_EQUAL("1.2345", Fmi::to_string(1.2345));
  BOOST_CHECK_EQUAL("1.23457", Fmi::to_string(1.234567890123456789f));
  BOOST_CHECK_EQUAL("1.23457", Fmi::to_string(1.234567890123456789));

  BOOST_CHECK_EQUAL("1.235", Fmi::to_string("%.3f", 1.234567890123456789f));
  BOOST_CHECK_EQUAL("1.235", Fmi::to_string("%.3f", 1.234567890123456789));
}

BOOST_AUTO_TEST_CASE(to_iso_string)
{
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  ptime time1(date(2002, Jan, 1), time_duration(1, 2, 3));
  BOOST_CHECK_EQUAL("20020101T010203", Fmi::to_iso_string(time1));

  ptime time2(date(2002, Jan, 1), time_duration(1, 2, 3) + milliseconds(4));
  BOOST_CHECK_EQUAL("20020101T010203,004000", Fmi::to_iso_string(time2));
}

BOOST_AUTO_TEST_CASE(to_iso_extended_string)
{
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  ptime time1(date(2002, Jan, 1), time_duration(1, 2, 3));
  BOOST_CHECK_EQUAL("2002-01-01T01:02:03", Fmi::to_iso_extended_string(time1));

  ptime time2(date(2002, Jan, 1), time_duration(1, 2, 3) + milliseconds(4));
  BOOST_CHECK_EQUAL("2002-01-01T01:02:03,004000", Fmi::to_iso_extended_string(time2));
}

BOOST_AUTO_TEST_CASE(ascii_tolower)
{
  BOOST_TEST_MESSAGE(" + Fmi::ascii_tolower()");

  std::string input = "ABDCEFGHIJKLMNOPQRSTUVWXYZ";

  Fmi::ascii_tolower(input);

  const std::string correct_result = "abdcefghijklmnopqrstuvwxyz";

  BOOST_CHECK_EQUAL(input, correct_result);
}

BOOST_AUTO_TEST_CASE(ascii_toupper)
{
  BOOST_TEST_MESSAGE(" + Fmi::ascii_toupper()");

  std::string input = "abdcefghijklmnopqrstuvwxyz";

  Fmi::ascii_toupper(input);

  const std::string correct_result = "ABDCEFGHIJKLMNOPQRSTUVWXYZ";

  BOOST_CHECK_EQUAL(input, correct_result);
}

// ======================================================================