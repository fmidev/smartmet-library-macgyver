// ======================================================================
/*!
 * \file
 * \brief Regression tests for TypeMap.h
 */
// ======================================================================

#include "TypeMap.h"
#include <functional>
#include <iostream>
#include <boost/test/included/unit_test.hpp>

using namespace boost::unit_test;

test_suite* init_unit_test_suite(int argc, char* argv[])
{
  const char* name = "Fmi::TypeMap tester";
  // unit_test_log.set_threshold_level(log_test_units);
  unit_test_log.set_threshold_level(log_messages);
  framework::master_test_suite().p_name.value = name;
  BOOST_TEST_MESSAGE("");
  BOOST_TEST_MESSAGE(name);
  BOOST_TEST_MESSAGE(std::string(std::strlen(name), '='));
  return NULL;
}

BOOST_AUTO_TEST_CASE(test_with_boost_any)
{
    BOOST_TEST_MESSAGE("Testing with boost::any");
    Fmi::TypeMap<std::function<std::string(const boost::any&)> > W;
    W.add<int>(
        [](const boost::any& x) -> std::string
        {
            return "INT: " + std::to_string(boost::any_cast<int>(x));
        });
    W.add<std::string>(
        [](const boost::any& x) -> std::string
        {
            return "STRING: '" + boost::any_cast<std::string>(x) + "'";
        });
    boost::any foo = int(42), bar(std::string("bar")), bad(3.14);
    std::string result;
    BOOST_REQUIRE_NO_THROW(result = W [foo] (foo));
    BOOST_CHECK_EQUAL(result, std::string("INT: 42"));

    BOOST_REQUIRE_NO_THROW(result = W [bar] (bar));
    BOOST_CHECK_EQUAL(result, std::string("STRING: 'bar'"));

    // No function defined for contained type (should throw)
    BOOST_REQUIRE_THROW(W [bad] (bad), Fmi::Exception);

    // Not implemented yet
    //BOOST_REQUIRE_NO_THROW(W(foo));
}

BOOST_AUTO_TEST_CASE(test_with_boost_variant)
{
    BOOST_TEST_MESSAGE("Testing with boost::variant<>");
    typedef boost::variant<int, double, std::string> MyVariant;
    Fmi::TypeMap<std::function<std::string(const MyVariant&)> > W;
    W.add<int>(
        [](const MyVariant& x) -> std::string
        {
            return "INT: " + std::to_string(boost::get<int>(x));
        });
    W.add<std::string>(
        [](const MyVariant& x) -> std::string
        {
            return "STRING: '" + boost::get<std::string>(x) + "'";
        });
    MyVariant a = int(42), b = 3.14, c = std::string("foo");

    std::string result;
    BOOST_REQUIRE_NO_THROW(result = W [a] (a));
    BOOST_CHECK_EQUAL(result, std::string("INT: 42"));

    BOOST_REQUIRE_NO_THROW(result = W [c] (c));
    BOOST_CHECK_EQUAL(result, std::string("STRING: 'foo'"));

    BOOST_REQUIRE_THROW(W [b] (b), Fmi::Exception);
}


