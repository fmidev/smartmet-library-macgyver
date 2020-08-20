// ======================================================================
/*!
 * \file
 * \brief Regression tests for Fmi::AsyncTaskGroup
 */
// ======================================================================

#include "AsyncTaskGroup.h"
#include <regression/tframe.h>
#include <atomic>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "TypeName.h"

using namespace boost::posix_time;

namespace AsyncTaskGroupTest {

  // Verify that there is no corruption when wait() is not called
  void not_waiting_for_result()
  {
    std::unique_ptr<std::string> test(new std::string);
    std::unique_ptr<Fmi::AsyncTaskGroup> tg(new Fmi::AsyncTaskGroup);
    tg->add("not waiting for result",
	   [&]()
	   {
	     std::this_thread::sleep_for(std::chrono::milliseconds(50));
	     *test = "foobar";
	   });
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    tg.reset();
    test.reset();
    TEST_PASSED();
  }

  void single_task()
  {
    std::atomic<int> test(0);
    Fmi::AsyncTaskGroup tg;
    tg.add("", [&]() { test++; });
    tg.wait();
    if (test != 1) { TEST_FAILED("Not expected result"); }
    TEST_PASSED();
  }

  void several_tasks()
  {
    std::atomic<int> test(0);
    Fmi::AsyncTaskGroup tg;
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
    Fmi::AsyncTaskGroup tg;
    tg.on_task_error([&](const std::string& name) {
	exception_name = Fmi::current_exception_type();
	task_name = name;
      });
    tg.add("foobar", []() { throw TestException(); });
    tg.wait();
    if (exception_name != "AsyncTaskGroupTest::TestException") { TEST_FAILED("Did not get exception name"); }
    if (task_name != "foobar") { TEST_FAILED("Did not get task name"); }
    TEST_PASSED();
  }

  void large_number_of_tasks()
  {
    bool failed = false;
    std::atomic<int> test(0);
    Fmi::AsyncTaskGroup tg(10);
    for (int i = 0; i < 100; i++) {
      tg.add("test",
	     [&](){
	       std::this_thread::sleep_for(std::chrono::milliseconds(10));
	       test++;
	     });
    }
    if (not failed and tg.get_num_active_tasks() > 10) {
      TEST_FAILED("Too many parallel tasks"); failed = true;
    }
    tg.wait();
    if (test != 100) {
      TEST_FAILED("Not all tasks executed");
    }
    TEST_PASSED();
  }

  void test_interrupting_1()
  {
    std::unique_ptr<Fmi::AsyncTaskGroup> tg(new Fmi::AsyncTaskGroup(10));
    for (int i = 0; i < 5; i++) {
      tg->add("test", [] () { boost::this_thread::sleep_for(boost::chrono::milliseconds(1000)); });
    }

    const ptime t1 = microsec_clock::universal_time();
    tg->stop();
    tg->wait();
    const ptime t2 = microsec_clock::universal_time();
    tg.reset();
    const ptime t3 = microsec_clock::universal_time();
    //std::cout << '\n' << (t2-t1) << std::endl;
    //std::cout << '\n' << (t3-t1) << std::endl;
    if ((t2-t1).total_milliseconds() > 50) {
        TEST_FAILED("Did not stop in time");
    }
    if ((t3-t1).total_milliseconds() > 50) {
        TEST_FAILED("Stuck in destructor for too long time");
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
      TEST(test_interrupting_1);
    }
  };


}

int main(void)
{
  std::cout << std::endl << "AsyncTaskGroup tester" << std::endl << "=====================" << std::endl;
  AsyncTaskGroupTest::tests t;
  return t.run();
}
