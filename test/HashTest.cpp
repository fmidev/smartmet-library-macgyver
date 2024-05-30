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

BOOST_AUTO_TEST_CASE(variadic_hash_template_1)
{
  BOOST_TEST_MESSAGE(" + Fmi::hash variadic template function (1/2)");
  const std::string str = "Hello, World!";
  const std::vector<double> vec = {1.0, 2.0, 3.0};
  const int64_t i = 42;

  std::size_t hash1 = Fmi::hash_value(str);
  Fmi::hash_combine(hash1, Fmi::hash_value(vec));
  Fmi::hash_combine(hash1, Fmi::hash_value(i));

  std::size_t hash2 = Fmi::hash(str, vec, i);
  BOOST_CHECK_EQUAL(hash1, hash2);

  // Must check also that the variadic template works with a single argument
  BOOST_CHECK_EQUAL(Fmi::hash(str), Fmi::hash_value(str));
}

BOOST_AUTO_TEST_CASE(variadic_hash_template_2)
{
  BOOST_TEST_MESSAGE(" + Fmi::hash variadic template function (2/2)");
  const std::string str = "Hello, World!";
  const std::vector<double> vec = {1.0, 2.0, 3.0};
  const int64_t i = 42;

  std::size_t hash1 = Fmi::hash_value(str);
  Fmi::hash_combine(hash1, Fmi::hash_value(vec));
  Fmi::hash_combine(hash1, Fmi::hash_value(i));

  // Case when one of hashes is already calculated (e.g. returned of method call of some class)
  std::size_t hash2 = Fmi::hash(str, Fmi::HashValue(Fmi::hash_value(vec)), i);
  BOOST_CHECK_EQUAL(hash1, hash2);
}
