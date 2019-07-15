// ======================================================================
/*!
 * \file
 * \brief Regression tests for Fmi::TaskGroup
 */
// ======================================================================

#include "TaskGroup.h"
#include <regression/tframe.h>
#include <atomic>
#include <iostream>
#include <memory>
#include <string>
#include "TypeName.h"

namespace TaskGroupTest {

  // Verify that there is no corruption when wait() is not called
  void not_waiting_for_result()
  {
    std::unique_ptr<std::string> test(new std::string);
    std::unique_ptr<Fmi::TaskGroup> tg(new Fmi::TaskGroup);
    tg->add("not waiting for result",
	   [&]()
	   {
	     //std::cout << __PRETTY_FUNCTION__ << ":" << __LINE__ << std::endl;
	     std::this_thread::sleep_for(std::chrono::milliseconds(50));
	     //std::cout << __PRETTY_FUNCTION__ << ":" << __LINE__ << std::endl;
	     *test = "foobar";
	   });
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    tg.reset();
    test.reset();
    //std::cout << __PRETTY_FUNCTION__ << ":" << __LINE__ << std::endl;
    TEST_PASSED();
  }

  void single_task()
  {
    std::atomic<int> test(0);
    Fmi::TaskGroup tg;
    tg.add("", [&]() { test++; });
    tg.wait();
    if (test != 1) { TEST_FAILED("Not expected result"); }
    TEST_PASSED();
  }

  void several_tasks()
  {
    std::atomic<int> test(0);
    Fmi::TaskGroup tg;
    for (int i = 0; i < 10; i++) {
      tg.add("", [&]() { test++; });
    }
    tg.wait();
    if (test != 10) { TEST_FAILED("Not expected result"); }
    TEST_PASSED();
  }

  struct TestException
  {
  };

  void task_throws_not_std_exception()
  {
    std::string exception_name;
    std::string task_name;
    Fmi::TaskGroup tg;
    tg.on_task_error([&](const std::string& name) {
	exception_name = Fmi::current_exception_type();
	task_name = name;
      });
    tg.add("foobar", []() { throw TestException(); });
    tg.wait();
    if (exception_name != "TaskGroupTest::TestException") { TEST_FAILED("Did not get exception name"); }
    if (task_name != "foobar") { TEST_FAILED("Did not get task name"); }
    TEST_PASSED();
  }

  void large_number_of_tasks()
  {
    bool failed = false;
    std::atomic<int> test(0);
    Fmi::TaskGroup tg(10);
    for (int i = 0; i < 100; i++) {
      tg.add("test",
	     [&](){
	       std::this_thread::sleep_for(std::chrono::milliseconds(10));
	       test++;
	     });
    }
    if (not failed and tg.get_num_tasks() > 10) {
      TEST_FAILED("Too many parallel tasks"); failed = true;
    }
    tg.wait();
    if (test != 100) {
      TEST_FAILED("Not all tasks executed");
    }
    TEST_PASSED();
  }

  // ----------------------------------------------------------------------
  /*!
   * The actual test suite
   */
  // ----------------------------------------------------------------------

  class tests : public tframe::tests
  {
    virtual const char* error_message_prefix() const { return "\n\t"; }

    void test(void)
    {
      TEST(not_waiting_for_result);
      TEST(single_task);
      TEST(several_tasks);
      TEST(task_throws_not_std_exception);
      TEST(large_number_of_tasks);
    }
  };


}

int main(void)
{
  std::cout << std::endl << "TaskGroup tester" << std::endl << "=====================" << std::endl;
  TaskGroupTest::tests t;
  return t.run();
}
