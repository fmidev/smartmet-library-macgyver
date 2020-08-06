#include "DirectoryMonitor.h"
#include <chrono>
#include <thread>
#include <condition_variable>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <regression/tframe.h>

namespace fs = boost::filesystem;
namespace pt = boost::posix_time;

namespace DirectoryMonitorTests {

void stopping_test()
{
    volatile bool started = false;
    std::mutex m;
    std::condition_variable c;
    Fmi::DirectoryMonitor monitor;
    monitor.watch(".",
        [&c, &m, &started](Fmi::DirectoryMonitor::Watcher, const fs::path&, const boost::regex, const Fmi::DirectoryMonitor::Status&) {
            std::unique_lock<std::mutex> lock(m);
            started = true;
            c.notify_all();
        },
        [](Fmi::DirectoryMonitor::Watcher, const fs::path&, const boost::regex&, const std::string&) {},
        10);
    const pt::ptime start = pt::microsec_clock::universal_time();
    std::thread t([&monitor]() { monitor.run(); });

    {
        std::unique_lock<std::mutex> lock(m);
        if (not started) {
            c.wait_for(lock, std::chrono::seconds(5));
        }
    }

    //const pt::ptime t1 = pt::microsec_clock::universal_time();
    //std::cout << t1 - start << std::endl;
    monitor.stop();
    t.join();

    const pt::ptime done = pt::microsec_clock::universal_time();
    if ((done - start).total_milliseconds() > 500) {
        std::ostringstream tmp;
        tmp << (done - start);
        TEST_FAILED("Stopping Fmi::DirectoryMonitor takes too long (" + tmp.str() + ")");
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
  }

};  // class tests

}  // namespace DirectoryMonitorTests

int main(void)
{
  std::cout << std::endl << "DirectoryMonitor tester" << std::endl
                         << "=======================" << std::endl;
  DirectoryMonitorTests::tests t;
  return t.run();
}
