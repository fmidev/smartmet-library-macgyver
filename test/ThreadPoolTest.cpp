#include "ThreadPool.h"
#include <atomic>
#include <iostream>
#include <boost/chrono.hpp>
#include <boost/thread.hpp>
#include <boost/test/included/unit_test.hpp>

using namespace boost::unit_test;

test_suite* init_unit_test_suite(int argc, char* argv[])
{
  const char* name = "Fmi::ThreadPool tester";
  // unit_test_log.set_threshold_level(log_test_units);
  unit_test_log.set_threshold_level(log_messages);
  framework::master_test_suite().p_name.value = name;
  BOOST_TEST_MESSAGE("");
  BOOST_TEST_MESSAGE(name);
  BOOST_TEST_MESSAGE(std::string(std::strlen(name), '='));
  std::setlocale(LC_ALL, "C");
  return nullptr;
}

BOOST_AUTO_TEST_CASE(test_thread_pool_1)
{
  std::atomic<int> count = 0;
  Fmi::ThreadPool::ThreadPool pool(4, 6);
  pool.setGracefulShutdown(true);
  pool.start();

  for (int i = 0; i < 10; ++i)
  {
    bool inserted = false;
    do
    {
        inserted = pool.schedule(
            [&count]() {
                boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
                count++;
            });
        //std::cout << "i=" << i << " inserted=" << inserted << std::endl;
        if (!inserted)
        {
            boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
        }
    } while (!inserted);
  }

  pool.join();
  pool.shutdown();
  BOOST_CHECK_EQUAL(count, 10);
}

// A graceful shutdown with a timeout must not wait forever for a stuck task: once the
// timeout elapses it falls back to interrupting the workers. The task sleeps with
// boost::this_thread::sleep_for, which is a boost interruption point (std::this_thread's
// sleep is not), so interrupt() actually aborts it.
BOOST_AUTO_TEST_CASE(shutdown_timeout_interrupts_stuck_task)
{
  std::atomic<bool> started{false};
  std::atomic<bool> completed{false};

  Fmi::ThreadPool::ThreadPool pool(1, 10);
  pool.setGracefulShutdown(true);
  pool.start();

  bool inserted = pool.schedule(
      [&started, &completed]()
      {
        started = true;
        boost::this_thread::sleep_for(boost::chrono::seconds(10));
        completed = true;  // must NOT be reached: the task is interrupted first
      });
  BOOST_REQUIRE(inserted);

  // Make sure a worker has actually picked up the task and is sleeping inside it.
  const auto start_deadline = boost::chrono::steady_clock::now() + boost::chrono::seconds(2);
  while (!started && boost::chrono::steady_clock::now() < start_deadline)
    boost::this_thread::sleep_for(boost::chrono::milliseconds(5));
  BOOST_REQUIRE_MESSAGE(started, "worker did not start task within 2 seconds");
  const auto t0 = boost::chrono::steady_clock::now();
  pool.shutdown(0.5);  // graceful, but bail out after 0.5 s and interrupt
  const auto elapsed_ms = boost::chrono::duration_cast<boost::chrono::milliseconds>(
                              boost::chrono::steady_clock::now() - t0)
                              .count();

  BOOST_TEST_MESSAGE("shutdown(0.5) with a 10 s task returned after " << elapsed_ms << " ms");
  BOOST_CHECK(!completed);           // task was interrupted, not run to completion
  BOOST_CHECK_LT(elapsed_ms, 5000);  // did not wait for the full 10 s task
  BOOST_CHECK_GE(elapsed_ms, 300);   // did wait roughly the timeout before interrupting
}

// With a generous timeout the currently running task still finishes gracefully; the
// timeout only bounds the wait, it does not cut a task short unnecessarily.
BOOST_AUTO_TEST_CASE(shutdown_timeout_allows_task_to_finish)
{
  std::atomic<bool> started{false};
  std::atomic<bool> completed{false};

  Fmi::ThreadPool::ThreadPool pool(1, 10);
  pool.setGracefulShutdown(true);
  pool.start();

  bool inserted = pool.schedule(
      [&started, &completed]()
      {
        started = true;
        boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
        completed = true;
      });
  BOOST_REQUIRE(inserted);

  while (!started)
    boost::this_thread::sleep_for(boost::chrono::milliseconds(5));

  const auto t0 = boost::chrono::steady_clock::now();
  pool.shutdown(5.0);  // generous timeout: the 200 ms task finishes before it elapses
  const auto elapsed_ms = boost::chrono::duration_cast<boost::chrono::milliseconds>(
                              boost::chrono::steady_clock::now() - t0)
                              .count();

  BOOST_TEST_MESSAGE("shutdown(5.0) with a 200 ms task returned after " << elapsed_ms << " ms");
  BOOST_CHECK(completed);            // graceful shutdown let the running task finish
  BOOST_CHECK_LT(elapsed_ms, 4000);  // returned well before the timeout
}
