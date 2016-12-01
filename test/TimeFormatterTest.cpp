// ======================================================================
/*!
 * \file
 * \brief Regression tests for class TimeFormatter
 */
// ======================================================================

#include "TimeFormatter.h"
#include <regression/tframe.h>

// Protection against conflicts with global functions
namespace TimeFormatterTest
{
// ----------------------------------------------------------------------

void format_timestamp()
{
  using namespace Fmi;
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  std::string ok, res;

  boost::shared_ptr<TimeFormatter> tf(TimeFormatter::create("timestamp"));

  ok = "200701020500";
  if ((res = tf->format(ptime(date(2007, 1, 2), hours(5) + minutes(0)))) != ok)
    TEST_FAILED("Expected " + ok + ", got " + res);

  ok = "200002280515";
  if ((res = tf->format(ptime(date(2000, 2, 28), hours(5) + minutes(15)))) != ok)
    TEST_FAILED("Expected " + ok + ", got " + res);

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void format_epoch()
{
  using namespace Fmi;
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  std::string ok, res;

  boost::shared_ptr<TimeFormatter> tf(TimeFormatter::create("epoch"));

  // date +%s --date="2007-01-02 05:00:00 UTC"  --> 1167714000
  ok = "1167714000";
  if ((res = tf->format(ptime(date(2007, 1, 2), hours(5) + minutes(0)))) != ok)
    TEST_FAILED("Expected " + ok + ", got " + res);

  // date +%s --date="2000-02-28 05:15:00 UTC" --> 951714900
  ok = "951714900";
  if ((res = tf->format(ptime(date(2000, 2, 28), hours(5) + minutes(15)))) != ok)
    TEST_FAILED("Expected " + ok + ", got " + res);

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void format_iso()
{
  using namespace Fmi;
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  std::string ok, res;

  boost::shared_ptr<TimeFormatter> tf(TimeFormatter::create("iso"));

  ok = "20070102T050000";
  if ((res = tf->format(ptime(date(2007, 1, 2), hours(5) + minutes(0)))) != ok)
    TEST_FAILED("Expected " + ok + ", got " + res);

  ok = "20000228T051500";
  if ((res = tf->format(ptime(date(2000, 2, 28), hours(5) + minutes(15)))) != ok)
    TEST_FAILED("Expected " + ok + ", got " + res);

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void format_sql()
{
  using namespace Fmi;
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  std::string ok, res;

  boost::shared_ptr<TimeFormatter> tf(TimeFormatter::create("sql"));

  ok = "2007-01-02 05:00:00";
  if ((res = tf->format(ptime(date(2007, 1, 2), hours(5) + minutes(0)))) != ok)
    TEST_FAILED("Expected " + ok + ", got " + res);

  ok = "2000-02-28 05:15:00";
  if ((res = tf->format(ptime(date(2000, 2, 28), hours(5) + minutes(15)))) != ok)
    TEST_FAILED("Expected " + ok + ", got " + res);

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void format_xml()
{
  using namespace Fmi;
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  std::string ok, res;

  boost::shared_ptr<TimeFormatter> tf(TimeFormatter::create("xml"));

  ok = "2007-01-02T05:00:00";
  if ((res = tf->format(ptime(date(2007, 1, 2), hours(5) + minutes(0)))) != ok)
    TEST_FAILED("Expected " + ok + ", got " + res);

  ok = "2000-02-28T05:15:00";
  if ((res = tf->format(ptime(date(2000, 2, 28), hours(5) + minutes(15)))) != ok)
    TEST_FAILED("Expected " + ok + ", got " + res);

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void format_http()
{
  using namespace Fmi;
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  std::string ok, res;

  boost::shared_ptr<TimeFormatter> tf(TimeFormatter::create("http"));

  ok = "Sun, 06 Nov 1994 08:49:37 GMT";
  if ((res = tf->format(ptime(date(1994, 11, 6), hours(8) + minutes(49) + seconds(37)))) != ok)
    TEST_FAILED("Expected " + ok + ", got " + res);

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
    TEST(format_timestamp);
    TEST(format_iso);
    TEST(format_sql);
    TEST(format_xml);
    TEST(format_epoch);
    TEST(format_http);
  }
};

}  // namespace TimeFormatterTest

//! The main program
int main(void)
{
  using namespace std;
  cout << endl << "TimeFormatter tester" << endl << "====================" << endl;
  TimeFormatterTest::tests t;
  return t.run();
}

// ======================================================================
