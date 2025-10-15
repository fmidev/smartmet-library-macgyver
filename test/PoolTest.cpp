#include "Pool.h"
#include <boost/test/included/unit_test.hpp>
#include <iostream>

using namespace boost::unit_test;
using namespace std::string_literals;

test_suite* init_unit_test_suite(int argc, char* argv[])
{
  const char* name = "Pool tester";
  unit_test_log.set_threshold_level(log_messages);
  framework::master_test_suite().p_name.value = name;
  BOOST_TEST_MESSAGE("");
  BOOST_TEST_MESSAGE(name);
  BOOST_TEST_MESSAGE(std::string(std::strlen(name), '='));
  return nullptr;
}

BOOST_AUTO_TEST_CASE(simple_test_1)
{
  Fmi::Pool<Fmi::PoolInitType::Sequential, int> pool(2, 4);
  BOOST_CHECK_EQUAL(int(pool.size()), 2);
  BOOST_CHECK_EQUAL(int(pool.in_use()), 0);
  auto ptr = pool.get();
  BOOST_CHECK(ptr.get() != nullptr);
  BOOST_CHECK_EQUAL(int(pool.in_use()), 1);
  BOOST_CHECK_EQUAL(int(pool.size()), 2);
  ptr.reset();
  BOOST_CHECK_EQUAL(int(pool.in_use()), 0);
  BOOST_CHECK_EQUAL(int(pool.size()), 2);

  //pool.dumpInfo(std::cout);
}

BOOST_AUTO_TEST_CASE(simple_test_2)
{
  Fmi::Pool<Fmi::PoolInitType::Parallel, int> pool(2, 4);
  BOOST_CHECK_EQUAL(int(pool.size()), 2);
  BOOST_CHECK_EQUAL(int(pool.in_use()), 0);
  auto ptr = pool.get();
  BOOST_CHECK(ptr.get() != nullptr);
  BOOST_CHECK_EQUAL(int(pool.in_use()), 1);
  BOOST_CHECK_EQUAL(int(pool.size()), 2);
  ptr.reset();
  BOOST_CHECK_EQUAL(int(pool.in_use()), 0);
  BOOST_CHECK_EQUAL(int(pool.size()), 2);

  //pool.dumpInfo(std::cout);
}

BOOST_AUTO_TEST_CASE(simple_test_3)
{
  Fmi::Pool<Fmi::PoolInitType::Sequential, int> pool(10, 20);
  BOOST_CHECK_EQUAL(int(pool.size()), 10);
  BOOST_CHECK_EQUAL(int(pool.in_use()), 0);

  std::vector<decltype(pool.get())> items;
  std::generate_n(std::back_insert_iterator(items), 15, [&pool]() { return pool.get(); });

  BOOST_CHECK_EQUAL(int(pool.in_use()), 15);
  BOOST_CHECK_EQUAL(int(pool.size()), 15);

  for (int i = 14; i >= 10; i--)
    items[i].reset();

  BOOST_CHECK_EQUAL(int(pool.in_use()), 10);
  BOOST_CHECK_EQUAL(int(pool.size()), 15);

  //pool.dumpInfo(std::cout);
}

BOOST_AUTO_TEST_CASE(factory_method_test)
{
  Fmi::Pool<Fmi::PoolInitType::Sequential, std::string, std::string> pool(
    [](const std::string& str) { return std::make_unique<std::string>(str); }, 2, 4, "hello"s);
  BOOST_CHECK_EQUAL(int(pool.size()), 2);
  BOOST_CHECK_EQUAL(int(pool.in_use()), 0);
  auto ptr = pool.get();
  BOOST_CHECK(ptr.get() != nullptr);
  BOOST_CHECK_EQUAL(int(pool.in_use()), 1);
  BOOST_CHECK_EQUAL(int(pool.size()), 2);
  BOOST_CHECK_EQUAL(*ptr.get(), "hello"s);
  ptr.reset();
  BOOST_CHECK_EQUAL(int(pool.in_use()), 0);
  BOOST_CHECK_EQUAL(int(pool.size()), 2);

  //pool.dumpInfo(std::cout);
}

namespace
{
  struct TestObj1
  {
    TestObj1(const std::string&) {}
    int value = 42;
  };

  struct TestObj2
  {
    TestObj2(const std::string&, int) {}
    int value = 42;
  };
}

BOOST_AUTO_TEST_CASE(constructor_with_1_argument)
{
  Fmi::Pool<Fmi::PoolInitType::Sequential, TestObj1, std::string> pool(2, 4, "foobar"s);
  decltype(pool.get()) ptr = pool.get();
  BOOST_CHECK_EQUAL(ptr->value, 42);
}

BOOST_AUTO_TEST_CASE(constructor_with_2_arguments)
{
  Fmi::Pool<Fmi::PoolInitType::Sequential, TestObj2, std::string, int> pool(2, 4, "foobar"s, 12);
  decltype(pool.get()) ptr = pool.get();
  BOOST_CHECK_EQUAL(ptr->value, 42);
}
