// ======================================================================
/*!
 * \file
 * \brief Regression tests for Hash.h
 */
// ======================================================================

#include "Hash.h"
#include <boost/test/included/unit_test.hpp>

using namespace boost::unit_test;

test_suite* init_unit_test_suite(int argc, char* argv[])
{
  const char* name = "Hash tester";
  unit_test_log.set_threshold_level(log_messages);
  framework::master_test_suite().p_name.value = name;
  BOOST_TEST_MESSAGE("");
  BOOST_TEST_MESSAGE(name);
  BOOST_TEST_MESSAGE(std::string(std::strlen(name), '='));
  return NULL;
}

BOOST_AUTO_TEST_CASE(hash_value)
{
  BOOST_TEST_MESSAGE(" + Fmi::hash_value()");

  // We excpect all results to be roughly 64 bits in size and greatly different
  BOOST_CHECK_EQUAL(14906445106408106831UL, Fmi::hash_value(false));
  BOOST_CHECK_EQUAL(11512047019328754865UL, Fmi::hash_value(true));
  BOOST_CHECK_EQUAL(14906445106408106831UL, Fmi::hash_value(0));
  BOOST_CHECK_EQUAL(11512047019328754865UL, Fmi::hash_value(1));
  BOOST_CHECK_EQUAL(15516417016181236671UL, Fmi::hash_value(2));
}

BOOST_AUTO_TEST_CASE(hash_combine)
{
  BOOST_TEST_MESSAGE(" + Fmi::hash_combine()");

  std::size_t hash = 0;
  Fmi::hash_combine(hash, Fmi::hash_value(false));
  BOOST_CHECK_EQUAL(8596974638888830062UL, hash);

  hash = 0;
  Fmi::hash_combine(hash, Fmi::hash_value(true));
  BOOST_CHECK_EQUAL(2432900628487897600UL, hash);
}
