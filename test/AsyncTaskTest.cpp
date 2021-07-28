// ======================================================================
/*!
 * \file
 * \brief Regression tests for Fmi::AsyncTask
 */
// ======================================================================

#include "AsyncTask.h"
#include "TypeName.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <regression/tframe.h>
#include <atomic>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

using namespace boost::posix_time;

namespace AsyncTaskTest
{
struct TestException
{
};

// Verify that there is no corruption when wait() is not called
void not_waiting_for_result()
{
  std::atomic<int> test(0);
  std::unique_ptr<Fmi::AsyncTask> task(new Fmi::AsyncTask("not waiting for result",
                                                          [&]()
                                                          {
                                                            for (int i = 0; i < 100; i++)
                                                            {
                                                              Fmi::AsyncTask::interruption_point();
                                                              std::this_thread::sleep_for(
                                                                  std::chrono::milliseconds(5));
                                                            }
                                                            test = 1;
                                                          }));
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ptime t1 = microsec_clock::universal_time();
  task.reset();
  ptime t2 = microsec_clock::universal_time();
  if ((t2 - t1).total_milliseconds() > 50)
  {
    // std::cout << "\n" << (t2 - t1) << std::endl;
    TEST_FAILED("Task not stopped as expected when wait for completion missing");
  }
  if (test != 0)
  {
    TEST_FAILED("Variable not updated as expected by task");
  }
  TEST_PASSED();
}

void not_waiting_for_result_2()
{
  std::unique_ptr<Fmi::AsyncTask> task(
      new Fmi::AsyncTask("not waiting for result", [&]() { throw TestException(); }));
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  task.reset();
  TEST_PASSED();
}

void single_task()
{
  std::atomic<int> test(0);
  Fmi::AsyncTask task("", [&]() { test++; });
  task.wait();
  if (test != 1)
  {
    TEST_FAILED("Not expected result");
  }
  if (task.get_status() != Fmi::AsyncTask::ok)
  {
    TEST_FAILED("Not expected test status");
  }
  TEST_PASSED();
}

void task_throws_not_std_exception()
{
  std::string exception_name;
  std::string task_name;
  Fmi::AsyncTask task("foobar", []() { throw TestException(); });
  try
  {
    task.wait();
    TEST_FAILED("Expection not thrown");
  }
  catch (const TestException&)
  {
  }
  catch (const tframe::failed&)
  {
    throw;
  }
  catch (...)
  {
    TEST_FAILED("Unexcpected exception of type '" + Fmi::current_exception_type() + "' thrown");
  }
  if (task.get_status() != Fmi::AsyncTask::failed)
  {
    TEST_FAILED("Not expected test status");
  }
  TEST_PASSED();
}

void test_cancel_task()
{
  Fmi::AsyncTask task("foobar",
                      []()
                      {
                        for (int i = 0; i < 100; i++)
                        {
                          std::this_thread::sleep_for(std::chrono::milliseconds(10));
                          Fmi::AsyncTask::interruption_point();
                        }
                      });

  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  ptime t1 = microsec_clock::universal_time();
  task.cancel();
  task.wait();
  ptime t2 = microsec_clock::universal_time();
  if (task.get_status() != Fmi::AsyncTask::interrupted)
  {
    TEST_FAILED("Not expected test status");
  }
  if ((t2 - t1).total_milliseconds() > 50)
  {
    TEST_FAILED("Canceling task took too long time");
  }
  TEST_PASSED();
}

void test_notify()
{
  std::mutex m;
  std::condition_variable cond;
  Fmi::AsyncTask task(
      "foobar",
      []() { std::this_thread::sleep_for(std::chrono::milliseconds(50)); },
      [&cond]() { cond.notify_all(); });

  std::unique_lock<std::mutex> lock(m);
  if (!cond.wait_for(lock, std::chrono::milliseconds(100), [&task]() { return task.ended(); }))
  {
    TEST_FAILED("Did not get notification about task end");
    task.cancel();
  }
  lock.unlock();

  TEST_PASSED();
}

// Stopping already finished task should not influence the status
void stop_attempt_after_task_end()
{
  std::mutex m;
  std::condition_variable cond;
  Fmi::AsyncTask task(
      "foobar",
      []()
      {
        /* Do nothing */
      },
      [&cond]() { cond.notify_all(); });

  std::unique_lock<std::mutex> lock(m);
  if (!cond.wait_for(lock, std::chrono::milliseconds(100), [&task]() { return task.ended(); }))
  {
    TEST_FAILED("Did not get notification about task end");
    task.cancel();
  }
  lock.unlock();

  task.cancel();

  task.wait();
  if (task.get_status() != Fmi::AsyncTask::ok)
  {
    TEST_FAILED("Not expected test status");
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
    Fmi::AsyncTask::silent = true;
    TEST(not_waiting_for_result);
    TEST(not_waiting_for_result_2);
    TEST(single_task);
    TEST(task_throws_not_std_exception);
    TEST(test_cancel_task);
    TEST(test_notify);
    TEST(stop_attempt_after_task_end);
  }
};

}  // namespace AsyncTaskTest

int main(void)
{
  std::cout << std::endl << "AsyncTask tester" << std::endl << "=====================" << std::endl;
  AsyncTaskTest::tests t;
  return t.run();
}
