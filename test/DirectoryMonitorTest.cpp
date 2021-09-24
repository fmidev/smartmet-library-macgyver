#include "DirectoryMonitor.h"
#include <boost/chrono.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/scope_exit.hpp>
#include <boost/thread.hpp>
#include <regression/tframe.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <thread>

namespace fs = boost::filesystem;
namespace pt = boost::posix_time;

namespace DirectoryMonitorTests
{
void stopping_test()
{
  std::atomic<bool> started(false);
  std::mutex m;
  std::condition_variable c;
  Fmi::DirectoryMonitor monitor;
  monitor.watch(
      ".",
      [](Fmi::DirectoryMonitor::Watcher,
         const fs::path&,
         const boost::regex,
         const Fmi::DirectoryMonitor::Status&) {},
      [](Fmi::DirectoryMonitor::Watcher, const fs::path&, const boost::regex&, const std::string&) {
      },
      10);
  const pt::ptime start = pt::microsec_clock::universal_time();
  std::thread t(
      [&monitor, &started, &c]()
      {
        started = true;
        c.notify_all();
        monitor.run();
      });

  {
    std::unique_lock<std::mutex> lock(m);
    if (not started)
    {
      c.wait_for(lock, std::chrono::seconds(5), [&started]() -> bool { return started; });
    }
  }

  // const pt::ptime t1 = pt::microsec_clock::universal_time();
  // std::cout << t1 - start << std::endl;
  monitor.stop();
  t.join();

  const pt::ptime done = pt::microsec_clock::universal_time();
  if ((done - start).total_milliseconds() > 500)
  {
    std::ostringstream tmp;
    tmp << (done - start);
    TEST_FAILED("Stopping Fmi::DirectoryMonitor takes too long (" + tmp.str() + ")");
  }

  TEST_PASSED();
}

void interruption_test()
{
  std::atomic<bool> started(false);
  std::mutex m;
  std::condition_variable c;
  Fmi::DirectoryMonitor monitor;
  monitor.watch(
      ".",
      [](Fmi::DirectoryMonitor::Watcher,
         const fs::path&,
         const boost::regex,
         const Fmi::DirectoryMonitor::Status&) {},
      [](Fmi::DirectoryMonitor::Watcher, const fs::path&, const boost::regex&, const std::string&) {
      },
      10);
  boost::thread task(
      [&monitor, &started, &c]()
      {
        started = true;
        c.notify_all();
        monitor.run();
      });

  BOOST_SCOPE_EXIT(&monitor, &task)
  {
    monitor.stop();
    task.join();
  }
  BOOST_SCOPE_EXIT_END;

  {
    std::unique_lock<std::mutex> lock(m);
    if (not started)
    {
      c.wait_for(lock, std::chrono::seconds(5), [&started]() -> bool { return started; });
    }
  }

  const pt::ptime t1 = pt::microsec_clock::universal_time();
  // std::cout << t1 - start << std::endl;
  task.interrupt();
  bool joined = task.try_join_for(boost::chrono::milliseconds(1000));
  if (joined)
  {
    const pt::ptime t2 = pt::microsec_clock::universal_time();
    const auto dt = 1.0e-6 * (t2-t1).total_microseconds();
    if (dt > 0.1)
    {
        std::cout << "\nStopping time dt seconds after interruption" << std::endl;
    }
  }
  else
  {
    const pt::ptime t2 = pt::microsec_clock::universal_time();
    const auto dt = 1.0e-6 * (t2-t1).total_microseconds();
    std::cout << "\nWait time dt seconds is too long" << std::endl;
    TEST_FAILED("Interrupting request did not stop directory monitor");
  }

  TEST_PASSED();
}

void wait_until_ready_test_1()
{
  Fmi::DirectoryMonitor monitor;
  monitor.watch(
      ".",
      [](Fmi::DirectoryMonitor::Watcher,
         const fs::path&,
         const boost::regex,
         const Fmi::DirectoryMonitor::Status&) {},
      [](Fmi::DirectoryMonitor::Watcher, const fs::path&, const boost::regex&, const std::string&) {
      },
      10);
  boost::thread task([&monitor]() { monitor.run(); });

  BOOST_SCOPE_EXIT(&monitor, &task)
  {
    monitor.stop();
    task.join();
  }
  BOOST_SCOPE_EXIT_END;

  bool ok = monitor.wait_until_ready();
  if (not ok)
  {
    TEST_FAILED("Waiting for first scan returned false");
  }

  TEST_PASSED();
}

void wait_until_ready_test_2()
{
  Fmi::DirectoryMonitor monitor;
  monitor.watch(
      ".",
      [](Fmi::DirectoryMonitor::Watcher,
         const fs::path&,
         const boost::regex,
         const Fmi::DirectoryMonitor::Status&) {},
      [](Fmi::DirectoryMonitor::Watcher, const fs::path&, const boost::regex&, const std::string&) {
      },
      10);

  boost::thread task([&monitor]() { monitor.run(); });

  BOOST_SCOPE_EXIT(&monitor, &task)
  {
    monitor.stop();
    task.join();
  }
  BOOST_SCOPE_EXIT_END;

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  monitor.stop();

  bool ok = monitor.wait_until_ready();
  if (ok)
  {
    TEST_FAILED("Monitor already ended. Should have returned false");
  }

  TEST_PASSED();
}

// ----------------------------------------------------------------------

// The actual test driver
class tests : public tframe::tests
{
  //! Overridden message separator
  virtual const char* error_message_prefix() const { return "\n\t"; }
  //! Main test suite
  void test(void)
  {
    TEST(stopping_test);
    TEST(interruption_test);
    TEST(wait_until_ready_test_1);
    TEST(wait_until_ready_test_2);
  }

};  // class tests

}  // namespace DirectoryMonitorTests

int main(void)
{
  std::cout << std::endl
            << "DirectoryMonitor tester" << std::endl
            << "=======================" << std::endl;
  DirectoryMonitorTests::tests t;
  return t.run();
}
