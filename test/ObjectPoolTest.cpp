#include "ObjectPool.h"
#include <boost/test/included/unit_test.hpp>
#include <iostream>

using namespace boost::unit_test;

test_suite* init_unit_test_suite(int argc, char* argv[])
{
  const char* name = "ObjectPool tester";
  unit_test_log.set_threshold_level(log_messages);
  framework::master_test_suite().p_name.value = name;
  BOOST_TEST_MESSAGE("");
  BOOST_TEST_MESSAGE(name);
  BOOST_TEST_MESSAGE(std::string(std::strlen(name), '='));
  return NULL;
}

using namespace Fmi;
using namespace boost::unit_test;

namespace
{
struct TestStruct
{
  std::string text;
};

boost::shared_ptr<TestStruct> create1() { return boost::shared_ptr<TestStruct>(new TestStruct); }
}  // namespace

BOOST_AUTO_TEST_CASE(test_single_object_reuse)
{
  unit_test_log.set_threshold_level(log_messages);
  BOOST_TEST_MESSAGE("+ [ObjectPool] Test whether object is reused");

  ObjectPool<TestStruct> test_pool(&create1, 5);
  BOOST_CHECK_EQUAL(0, (int)test_pool.size());
  auto p1 = test_pool.get();
  p1->text = "foo";
  BOOST_CHECK_EQUAL(1, (int)test_pool.size());
  p1.reset();
  auto p2 = test_pool.get();
  BOOST_CHECK_EQUAL(std::string("foo"), p2->text);
  BOOST_CHECK_EQUAL(1, (int)test_pool.size());
}

BOOST_AUTO_TEST_CASE(test_several_object_reuse)
{
  unit_test_log.set_threshold_level(log_messages);
  BOOST_TEST_MESSAGE("+ [ObjectPool] Test object reuse in case of 2 objects");

  ObjectPool<TestStruct> test_pool(&create1, 5);
  BOOST_CHECK_EQUAL(0, (int)test_pool.size());
  auto p1a = test_pool.get();
  p1a->text = "foo";
  BOOST_CHECK_EQUAL(1, (int)test_pool.size());
  auto p1b = test_pool.get();
  BOOST_CHECK_EQUAL(std::string(""), p1b->text);
  BOOST_CHECK_EQUAL(std::string("foo"), p1a->text);
  p1b->text = "bar";
  BOOST_CHECK_EQUAL(2, (int)test_pool.size());
  p1a.reset();
  auto p2 = test_pool.get();
  BOOST_CHECK_EQUAL(std::string("foo"), p2->text);
  BOOST_CHECK_EQUAL(2, (int)test_pool.size());
}

BOOST_AUTO_TEST_CASE(test_many_objects)
{
  unit_test_log.set_threshold_level(log_messages);
  BOOST_TEST_MESSAGE("+ [ObjectPool] Test releasing extra objects");

  int max_use = 0;
  ObjectPool<TestStruct> test_pool(&create1, 5);

  for (int i = 1; i <= 50; i++)
  {
    std::vector<boost::shared_ptr<TestStruct> > v;
    for (int k = 0; k < i and k < 51 - i; k++)
    {
      v.push_back(test_pool.get());
      max_use = std::max((int)test_pool.size(), max_use);
    }
  }

  BOOST_CHECK((int)test_pool.size() <= 6);
}
