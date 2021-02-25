#include "DirectoryMonitor.h"
#include <boost/chrono.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
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
  std::thread t([&monitor, &started, &c]() {
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
  boost::thread task([&monitor, &started, &c]() {
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
  task.interrupt();
  bool joined = task.try_join_for(boost::chrono::milliseconds(200));
  if (!joined)
  {
    TEST_FAILED("Interrupting request did not stop directory monitor");
    monitor.stop();
    task.join();
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
