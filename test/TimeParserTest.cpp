// ======================================================================
/*!
 * \file
 * \brief Regression tests for class TimeParser
 */
// ======================================================================

#include "TimeParser.h"
#include "TimeZoneFactory.h"
#include <regression/tframe.h>

#include "TimeParserTester.h"

template <typename T>
std::string tostring(const T& obj)
{
  std::ostringstream out;
  out << obj;
  return out.str();
}

// Protection against conflicts with global functions
namespace TimeParserTest
{
// ----------------------------------------------------------------------

void parse_timestamp()
{
  using namespace Fmi;
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  std::vector<Fmi::Test::TimeParseTest<boost::posix_time::ptime> > should_pass = {
      {"20070102T0500", ptime(date(2007, 1, 2), hours(5) + minutes(0))},
      {"20000228T0515", ptime(date(2000, 2, 28), hours(5) + minutes(15))}};

  std::vector<std::string> should_fail = {"foobar", "12345678901", "123456789012"};

  Fmi::Test::check_time_parse(should_pass, &TimeParser::parse_iso);
  Fmi::Test::check_time_parse_fail(should_fail, &TimeParser::parse_iso);

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void parse_epoch()
{
  using namespace Fmi;
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  std::vector<Fmi::Test::TimeParseTest<boost::posix_time::ptime> > should_pass = {
      // date +%s --date="2007-01-02 05:00:00 UTC"  --> 1167714000
      {"1167714000", ptime(date(2007, 1, 2), hours(5) + minutes(0))},
      // date +%s --date="2000-02-28 05:15:00 UTC" --> 951714900
      {"951714900", ptime(date(2000, 2, 28), hours(5) + minutes(15))},
      // date +%s --date="2019-08-23 07:05:33 UTC" --> 1566543933
      {"1566543933", ptime(date(2019, 8, 23), hours(7) + minutes(5) + seconds(33))},
      // date +%s --date="2019-08-22 07:01:59 UTC" --> 1566457319
      {"1566457319", ptime(date(2019, 8, 22), hours(7) + minutes(1) + seconds(59))},

      {"1400011680", ptime(date(2014, 05, 13), hours(20) + minutes(8))}};

  std::vector<std::string> should_fail = {"foobar", "12345678901"};

  Fmi::Test::check_time_parse(should_pass, &TimeParser::parse_epoch);
  Fmi::Test::check_time_parse_fail(should_fail, &TimeParser::parse_epoch);

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void parse_iso()
{
  using namespace Fmi;
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  std::vector<Fmi::Test::TimeParseTest<boost::posix_time::ptime> > should_pass = {
      {"20070102T050000", ptime(date(2007, 1, 2), hours(5) + minutes(0))},
      {"20000228T051500", ptime(date(2000, 2, 28), hours(5) + minutes(15))},
      {"2007-01-02T05:00:00", ptime(date(2007, 1, 2), hours(5) + minutes(0))},
      {"2000-02-28T05:15:00", ptime(date(2000, 2, 28), hours(5) + minutes(15))},
      {"2000-02-28T05:15:10", ptime(date(2000, 2, 28), hours(5) + minutes(15) + seconds(10))},
      {"2000-02-28T05:15:10Z", ptime(date(2000, 2, 28), hours(5) + minutes(15) + seconds(10))},
      {"2000-02-28T05:15:10+01:00", ptime(date(2000, 2, 28), hours(4) + minutes(15) + seconds(10))},
      {"2000-02-28T05:15:10+01:30", ptime(date(2000, 2, 28), hours(3) + minutes(45) + seconds(10))},
      {"2000-02-28T02:15:10-01:30", ptime(date(2000, 2, 28), hours(3) + minutes(45) + seconds(10))},
      {"2000-02-28T051510", ptime(date(2000, 2, 28), hours(5) + minutes(15) + seconds(10))},
      {"20000228T05:15:10", ptime(date(2000, 2, 28), hours(5) + minutes(15) + seconds(10))},
      {"2000-0228T0515:10", ptime(date(2000, 2, 28), hours(5) + minutes(15) + seconds(10))},
      {"20000228T", ptime(date(2000, 2, 28))},
      {"20000228T12", ptime(date(2000, 2, 28), hours(12))}  // 3 fractional seconds are ignored
      ,
      {"2000-02-28T12:00:00.321", ptime(date(2000, 2, 28), hours(12))},
      {"2000-02-28T12:00:00.321+03:00", ptime(date(2000, 2, 28), hours(9))}  // BRAINSTORM-480
      ,
      {"2022-05-19T21:00:00.000Z", ptime(date(2022,5,19), hours(21))},
      {"2015-06-09T16:00:00+03", ptime(date(2015, 6, 9), hours(13))},
      {"2015-06-09T16:00:00+03:00", ptime(date(2015, 6, 9), hours(13))},
      {"2015-06-09T16:00:00+0300", ptime(date(2015, 6, 9), hours(13))},
      {"2015-06-09T08:00:00-05:30", ptime(date(2015, 6, 9), hours(13) + minutes(30))}
#ifdef WE_DO_NOT_SUPPORT_FRACTIONS
      ,
      {"2000-02-28T05:15:10.123456",
       ptime(date(2000, 2, 28), hours(5) + minutes(15) + seconds(10) + microseconds(123456))},
      {"2000-02-28T05:15:10.987654Z",
       ptime(date(2000, 2, 28), hours(5) + minutes(15) + seconds(10) + microseconds(987654))}

#endif
      // the Gregorian calendar is officially introduced on 1582-10-15
      ,
      {"1582-10-15T00:00:00Z", ptime(date(1582, 10, 15), hours(0) + minutes(0) + seconds(0))}

      // BRAINSTORM-1071
      ,
      {"1900-1-1T00:00:00", ptime(date(1900, 1, 1))}};

  std::vector<std::string> should_fail = {
      "2000-02-28T12:00:00.3233",   // Expected to fail when given four fractional seconds
      "2000-02-28T12:00:00.33",     // Expected to fail when given two fractional seconds
      "2000-02-28T12:00:00.3"       // Expected to fail when given one fractional second
      "2015-06-09T16:00:00+00300",  // Should fail to parse '00300' timezone
      "2015-06-09T16:00:00--0300",  // Should fail to parse double minus in timezone
      "2015-06-09T16:00:00%0300",   // Should fail to parse invalid sign in timezone
      "foobar",
      "12345678901",
      "123456789012",
      "20160101T00000"  // Should fail to parse 20160101T00000 due to an extra zero (BRAINSTORM-696)
  };

  Fmi::Test::check_time_parse(should_pass, &TimeParser::parse_iso);
  Fmi::Test::check_time_parse_fail(should_fail, &TimeParser::parse_iso);

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void parse_fmi()
{
  using namespace Fmi;
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  std::vector<Fmi::Test::TimeParseTest<boost::posix_time::ptime> > should_pass = {
      {"200912120500", ptime(date(2009, 12, 12), hours(5) + minutes(0))},
      {"180001011500", ptime(date(1800, 1, 1), hours(15) + minutes(0))}};

  std::vector<std::string> should_fail = {
      "2009121205",      // Should fail to parse '2009121205', due to missing minute definition
      "200912120500334"  // Should fail to parse '200912120500334', due to extra tokens
  };

  Fmi::Test::check_time_parse(should_pass, &TimeParser::parse_fmi);
  Fmi::Test::check_time_parse_fail(should_fail, &TimeParser::parse_fmi);

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void try_parse_iso()
{
  using namespace Fmi;
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  bool utc;

  std::vector<Fmi::Test::TimeParseTest<boost::posix_time::ptime> > should_pass = {
      {"20070102T050000", ptime(date(2007, 1, 2), hours(5) + minutes(0))},
      {"20000228T051500", ptime(date(2000, 2, 28), hours(5) + minutes(15))},
      {"2000-02-28T05:15:10Z", ptime(date(2000, 2, 28), hours(5) + minutes(15) + seconds(10))},
      {"2000-02-28T05:15:10+01:00", ptime(date(2000, 2, 28), hours(4) + minutes(15) + seconds(10))},
      {"2000-02-28T05:15:10-01:30",
       ptime(date(2000, 2, 28), hours(6) + minutes(45) + seconds(10))}};

  Fmi::Test::check_time_parse(should_pass,
                              std::bind(&TimeParser::try_parse_iso, std::placeholders::_1, &utc));

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void parse_sql()
{
  using namespace Fmi;
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  std::vector<Fmi::Test::TimeParseTest<boost::posix_time::ptime> > should_pass = {
      {"2007-01-02 05:00:00", ptime(date(2007, 1, 2), hours(5) + minutes(0))},
      {"2000-02-28 05:15:00", ptime(date(2000, 2, 28), hours(5) + minutes(15))},
      {"2000-02-28 05", ptime(date(2000, 2, 28), hours(5))},
      {"2000-02-28 05:20:34", ptime(date(2000, 2, 28), hours(5) + minutes(20) + seconds(34))},
      {"2000-02-28", ptime(date(2000, 2, 28))},
      {"1900-01-01 00:00:0.0", ptime(date(1900, 1, 1))}};

  Fmi::Test::check_time_parse(should_pass, &TimeParser::parse_sql);

  const std::vector<std::string> invalid = {"foobar",
                                            "12345678901",
                                            "1970-0-1 00:00:00",
                                            "1970-13-1 00:00:00",
                                            "1970-001-1 00:00:00",
                                            "1970-1-0 00:00:00",
                                            "1970-1-32 00:00:00",
                                            "1970-1-1 24:00:00",
                                            "1970-1-1 00:60:00",
                                            "1970-1-1 23:00:60",
                                            "1970-1-1 23:059:00",
                                            "1970-01-01 00:000:00"};

  Fmi::Test::check_time_parse_fail(invalid, &TimeParser::parse_sql);

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void parse()
{
  using namespace Fmi;
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  ptime res, ok;

  ok = ptime(date(2007, 1, 2), hours(5) + minutes(0));
  if ((res = TimeParser::parse("200701020500")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2000, 2, 28), hours(5) + minutes(15));
  if ((res = TimeParser::parse("200002280515")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2007, 1, 2), hours(5) + minutes(0));
  if ((res = TimeParser::parse("20070102T050000")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2000, 2, 28), hours(5) + minutes(15));
  if ((res = TimeParser::parse("20000228T051500")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2007, 1, 2), hours(5) + minutes(0));
  if ((res = TimeParser::parse("2007-01-02 05:00:00")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2000, 2, 28), hours(5) + minutes(15));
  if ((res = TimeParser::parse("2000-02-28 05:15:00")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  // date +%s --date="2007-01-02 05:00:00 UTC"  --> 1167714000
  ok = ptime(date(2007, 1, 2), hours(5) + minutes(0));
  if ((res = TimeParser::parse("1167714000")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  // date +%s --date="2000-02-28 05:15:00 UTC" --> 951714900
  ok = ptime(date(2000, 2, 28), hours(5) + minutes(15));
  if ((res = TimeParser::parse("951714900")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2007, 1, 2), hours(5) + minutes(0));
  if ((res = TimeParser::parse("2007-01-02T05:00:00")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2000, 2, 28), hours(5) + minutes(15));
  if ((res = TimeParser::parse("2000-02-28T05:15:00")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2013, 5, 15), hours(13) + minutes(17) + seconds(23));
  if ((res = TimeParser::parse("20130515131723")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2013, 5, 15), hours(13) + minutes(17));
  if ((res = TimeParser::parse("201305151317")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  // date +%s --date="2019-08-23 07:05:33 UTC" --> 1566543933
  ok = ptime(date(2019, 8, 23), hours(7) + minutes(5) + seconds(33));
  if ((res = TimeParser::parse("1566543933")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  // date +%s --date="2019-08-22 07:01:59 UTC" --> 1566457319
  ok = ptime(date(2019, 8, 22), hours(7) + minutes(1) + seconds(59));
  if ((res = TimeParser::parse("1566457319")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  TimeParser::parse("-10h");
  TimeParser::parse("+12h");
  TimeParser::parse("+13H");
  TimeParser::parse("-14H");

  TimeParser::parse("-10m");
  TimeParser::parse("+12m");
  TimeParser::parse("+13M");
  TimeParser::parse("-14M");

  TEST_PASSED();
}

void parse_http()
{
  using namespace Fmi;
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  ptime res, ok;

  ok = ptime(date(1994, 11, 6), hours(8) + minutes(49) + seconds(37));

  if ((res = TimeParser::parse_http("Sun, 06 Nov 1994 08:49:37 GMT")) != ok)
    TEST_FAILED("Failed to parse Sun, 06 Nov 1994 08:49:37 GMT");

  if ((res = TimeParser::parse_http("Sunday, 06-Nov-94 08:49:37 GMT")) != ok)
    TEST_FAILED("Failed to parse Sunday, 06-Nov-94 08:49:37 GMT");

  if ((res = TimeParser::parse_http("Sun Nov  6 08:49:37 1994")) != ok)
    TEST_FAILED("Failed to parse Sun Nov  6 08:49:37 1994");

  TEST_PASSED();
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse offsets
 */
// ----------------------------------------------------------------------

void try_parse_offset()
{
  using namespace Fmi;
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  // Now rounded to closest minute
  ptime now = second_clock::universal_time();
  time_duration tnow = now.time_of_day();
  int secs = tnow.seconds();
  if (secs >= 30)
    now += seconds(60 - secs);
  else
    now -= seconds(secs);

  ptime res;

  res = TimeParser::try_parse_offset("0");
  if (res - now != minutes(0))
    TEST_FAILED("Failed to parse 0 correctly, got " + to_simple_string(res));

  res = TimeParser::try_parse_offset("0m");
  if (res - now != minutes(0))
    TEST_FAILED("Failed to parse 0m correctly, got " + to_simple_string(res));

  res = TimeParser::try_parse_offset("0h");
  if (res - now != minutes(0))
    TEST_FAILED("Failed to parse 0h correctly, got " + to_simple_string(res));

  res = TimeParser::try_parse_offset("0d");
  if (res - now != minutes(0))
    TEST_FAILED("Failed to parse 0d correctly, got " + to_simple_string(res));

  res = TimeParser::try_parse_offset("+1");
  if (res - now != minutes(1))
    TEST_FAILED("Failed to parse +1 correctly, got " + to_simple_string(res));

  res = TimeParser::try_parse_offset("+10m");
  if (res - now != minutes(10))
    TEST_FAILED("Failed to parse +10m correctly, got " + to_simple_string(res));

  res = TimeParser::try_parse_offset("+3h");
  if (res - now != hours(3))
    TEST_FAILED("Failed to parse +3h correctly, got " + to_simple_string(res));

  res = TimeParser::try_parse_offset("-24h");
  if (now - res != hours(24))
    TEST_FAILED("Failed to parse -24h correctly, got " + to_simple_string(res));

  res = TimeParser::try_parse_offset("+2d");
  if (res - now != hours(48))
    TEST_FAILED("Failed to parse +2d correctly, got " + to_simple_string(res));

  res = TimeParser::try_parse_offset("+2y");
  if (res - now != hours(2 * 365 * 24))
    TEST_FAILED("Failed to parse +2y correctly, got " + to_simple_string(res));

  res = TimeParser::try_parse_offset("+2w");
  if (res - now != hours(2 * 24 * 7))
    TEST_FAILED("Failed to parse +2w correctly, got " + to_simple_string(res));

  res = TimeParser::try_parse_offset("+2H");
  if (res - now != hours(2))
    TEST_FAILED("Failed to parse +2H correctly, got " + to_simple_string(res));

  res = TimeParser::try_parse_offset("1");
  if (!res.is_not_a_date_time())
    TEST_FAILED("Should have failed to parse 1 as an offset, got " + to_simple_string(res));

  res = TimeParser::try_parse_offset(" 123 ");
  if (!res.is_not_a_date_time())
    TEST_FAILED("Should have failed to parse ' 123 ' as an offset, got " + to_simple_string(res));

  res = TimeParser::try_parse_offset("-a");
  if (!res.is_not_a_date_time())
    TEST_FAILED("Should have failed to parse '-a' as an offset, got " + to_simple_string(res));

  TEST_PASSED();
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse offsets
 */
// ----------------------------------------------------------------------

void parse_offset()
{
  using namespace Fmi;
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  // Now rounded to closest minute
  ptime now = second_clock::universal_time();
  time_duration tnow = now.time_of_day();
  int secs = tnow.seconds();
  if (secs >= 30)
    now += seconds(60 - secs);
  else
    now -= seconds(secs);

  ptime res;

  res = TimeParser::parse("0");
  if (res - now != minutes(0))
    TEST_FAILED("Failed to parse 0 correctly, got " + to_simple_string(res));

  res = TimeParser::parse("0m");
  if (res - now != minutes(0))
    TEST_FAILED("Failed to parse 0m correctly, got " + to_simple_string(res));

  res = TimeParser::parse("0h");
  if (res - now != minutes(0))
    TEST_FAILED("Failed to parse 0h correctly, got " + to_simple_string(res));

  res = TimeParser::parse("0d");
  if (res - now != minutes(0))
    TEST_FAILED("Failed to parse 0d correctly, got " + to_simple_string(res));

  res = TimeParser::parse("+1");
  if (res - now != minutes(1))
    TEST_FAILED("Failed to parse +1 correctly, got " + to_simple_string(res));

  res = TimeParser::parse("+10m");
  if (res - now != minutes(10))
    TEST_FAILED("Failed to parse +10m correctly, got " + to_simple_string(res));

  res = TimeParser::parse("+3h");
  if (res - now != hours(3))
    TEST_FAILED("Failed to parse +3h correctly, got " + to_simple_string(res));

  res = TimeParser::parse("-24h");
  if (now - res != hours(24))
    TEST_FAILED("Failed to parse -24h correctly, got " + to_simple_string(res));

  res = TimeParser::parse("+2d");
  if (res - now != hours(48))
    TEST_FAILED("Failed to parse +2d correctly, got " + to_simple_string(res));

  res = TimeParser::parse("+2y");
  if (res - now != hours(2 * 365 * 24))
    TEST_FAILED("Failed to parse +2y correctly, got " + to_simple_string(res));

  res = TimeParser::parse("+2w");
  if (res - now != hours(2 * 24 * 7))
    TEST_FAILED("Failed to parse +2w correctly, got " + to_simple_string(res));

  res = TimeParser::parse("+2H");
  if (res - now != hours(2))
    TEST_FAILED("Failed to parse +2H correctly, got " + to_simple_string(res));

  TEST_PASSED();
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse durations
 */
// ----------------------------------------------------------------------

void try_parse_duration()
{
  using namespace Fmi;
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  time_duration res;

  res = TimeParser::try_parse_duration("0");
  if (res != seconds(0))
    TEST_FAILED("Failed to parse 0 correctly, got " + to_simple_string(res));

  res = TimeParser::try_parse_duration("+1");
  if (res != minutes(1))
    TEST_FAILED("Failed to parse +1 correctly, got " + to_simple_string(res));

  res = TimeParser::try_parse_duration("+10m");
  if (res != minutes(10))
    TEST_FAILED("Failed to parse +10m correctly, got " + to_simple_string(res));

  res = TimeParser::try_parse_duration("+3h");
  if (res != hours(3))
    TEST_FAILED("Failed to parse +3h correctly, got " + to_simple_string(res));

  res = TimeParser::try_parse_duration("-24h");
  if (res != hours(-24))
    TEST_FAILED("Failed to parse -24h correctly, got " + to_simple_string(res));

  res = TimeParser::try_parse_duration("+2d");
  if (res != hours(2 * 24))
    TEST_FAILED("Failed to parse +2d correctly, got " + to_simple_string(res));

  res = TimeParser::try_parse_duration("+2W");
  if (res != hours(7 * 2 * 24))
    TEST_FAILED("Failed to parse +2W correctly, got " + to_simple_string(res));

  res = TimeParser::try_parse_duration("123");
  if (!res.is_not_a_date_time())
    TEST_FAILED("Should have failed to parse 123 as duration, got " + to_simple_string(res));

  res = TimeParser::try_parse_duration(" -123 ");
  if (!res.is_not_a_date_time())
    TEST_FAILED("Should have failed to parse ' -123' as duration, got " + to_simple_string(res));

  TEST_PASSED();
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse durations
 */
// ----------------------------------------------------------------------

void parse_duration()
{
  using namespace Fmi;
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  time_duration res;

  res = TimeParser::parse_duration("0");
  if (res != seconds(0))
    TEST_FAILED("Failed to parse 0 correctly, got " + to_simple_string(res));

  res = TimeParser::parse_duration("+1");
  if (res != minutes(1))
    TEST_FAILED("Failed to parse +1 correctly, got " + to_simple_string(res));

  res = TimeParser::parse_duration("+10m");
  if (res != minutes(10))
    TEST_FAILED("Failed to parse +10m correctly, got " + to_simple_string(res));

  res = TimeParser::parse_duration("+3h");
  if (res != hours(3))
    TEST_FAILED("Failed to parse +3h correctly, got " + to_simple_string(res));

  res = TimeParser::parse_duration("-24h");
  if (res != hours(-24))
    TEST_FAILED("Failed to parse -24h correctly, got " + to_simple_string(res));

  res = TimeParser::parse_duration("+2d");
  if (res != hours(2 * 24))
    TEST_FAILED("Failed to parse +2d correctly, got " + to_simple_string(res));

  res = TimeParser::parse_duration("+2W");
  if (res != hours(7 * 2 * 24))
    TEST_FAILED("Failed to parse +2W correctly, got " + to_simple_string(res));

  TEST_PASSED();
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse ISO8601 durations
 */
// ----------------------------------------------------------------------

void parse_iso_duration()
{
  using namespace Fmi;
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  time_duration res;

  res = TimeParser::parse_iso_duration("P2W");
  if (res != hours(7 * 2 * 24))
    TEST_FAILED("Failed to parse P2W correctly, got " + to_simple_string(res));

  res = TimeParser::parse_iso_duration("P1D");
  if (res != hours(24))
    TEST_FAILED("Failed to parse P1D correctly, got " + to_simple_string(res));

  res = TimeParser::parse_iso_duration("P2D");
  if (res != hours(2 * 24))
    TEST_FAILED("Failed to parse P2D correctly, got " + to_simple_string(res));

  res = TimeParser::parse_iso_duration("PT0H");
  if (res != seconds(0))
    TEST_FAILED("Failed to parse PT0H correctly, got " + to_simple_string(res));

  res = TimeParser::parse_iso_duration("PT1M");
  if (res != minutes(1))
    TEST_FAILED("Failed to parse PT1M correctly, got " + to_simple_string(res));

  res = TimeParser::parse_iso_duration("PT10M");
  if (res != minutes(10))
    TEST_FAILED("Failed to parse PT10M correctly, got " + to_simple_string(res));

  res = TimeParser::parse_iso_duration("PT3H");
  if (res != hours(3))
    TEST_FAILED("Failed to parse PT3H correctly, got " + to_simple_string(res));

  res = TimeParser::parse_iso_duration("P1DT10H20M30S");
  if (res != hours(24) + hours(10) + minutes(20) + seconds(30))
    TEST_FAILED("Failed to parse P1DT10H20M30S correctly, got " + to_simple_string(res));

  TEST_PASSED();
}

// ----------------------------------------------------------------------
/*!
 * \brief Test time format recognizition
 */
// ----------------------------------------------------------------------

void looks()
{
  using namespace Fmi;

  std::string res;

  if ((res = TimeParser::looks("200701020500")) != "iso")
    TEST_FAILED("200701020500 should look like an ISO time stamp");

  if ((res = TimeParser::looks("20000228T051500")) != "iso")
    TEST_FAILED("20000228T051500 should look like an ISO date time");

  if ((res = TimeParser::looks("2007-01-02T05:00:00")) != "iso")
    TEST_FAILED("2007-01-02T05:00:00 should look like an ISO date time");

  if ((res = TimeParser::looks("2000-02-28T05:15:00")) != "iso")
    TEST_FAILED("2000-02-28T05:15:00 should look like an ISO date time");

  if ((res = TimeParser::looks("2000-02-28T05:15:10")) != "iso")
    TEST_FAILED("2000-02-28T05:15:10 should look like an ISO date time");

  if ((res = TimeParser::looks("2000-02-28T05:15:10Z")) != "iso")
    TEST_FAILED("2000-02-28T05:15:10Z should look like an ISO date time");

  if ((res = TimeParser::looks("2000-02-28T05:15:10+01:00")) != "iso")
    TEST_FAILED("2000-02-28T05:15:10Z should look like an ISO date time");

  if ((res = TimeParser::looks("2000-02-28T05:15:10-02")) != "iso")
    TEST_FAILED("2000-02-28T05:15:10Z should look like an ISO date time");

  if ((res = TimeParser::looks("2022-05-19T21:00:00.000Z")) != "iso")
    TEST_FAILED("2022-05-19T21:00:00.000Z should look like an ISO date time");

  if ((res = TimeParser::looks("2007-01-02 05:00:00")) != "sql")
    TEST_FAILED("2007-01-02 05:00:00 should look like a SQL date time");

  if ((res = TimeParser::looks("2000-02-28 05:15:00")) != "sql")
    TEST_FAILED("2000-02-28 05:15:00 should look like a SQL date time");

  if ((res = TimeParser::looks("1167714000")) != "epoch")
    TEST_FAILED("1167714000 should look like an epoch time");

  if ((res = TimeParser::looks("1357063111")) != "epoch")
    TEST_FAILED("1357063111 should look like an epoch time");

  if ((res = TimeParser::looks("+10")) != "offset")
    TEST_FAILED("+10 should look like an offset");
  if ((res = TimeParser::looks("-100")) != "offset")
    TEST_FAILED("-100 should look like an offset");
  if ((res = TimeParser::looks("+10m")) != "offset")
    TEST_FAILED("+10m should look like an offset");
  if ((res = TimeParser::looks("-100H")) != "offset")
    TEST_FAILED("-100H should look like an offset");

  if ((res = TimeParser::looks("1900-01-01 00:00:0.0")) != "sql")
    TEST_FAILED("1900-01-01 00:00:0.0 should look like SQL date time");

  try
  {
    res = TimeParser::looks("foobar");
    TEST_FAILED("foobar should not look like a date time");
  }
  catch (...)
  {
  }

  TEST_PASSED();
}

// ----------------------------------------------------------------------
/*!
 * \brief Test UTC time format recognizition
 */
// ----------------------------------------------------------------------

void looks_utc()
{
  using namespace Fmi;

  std::vector<Test::TimeParseTest<bool> > data =
      {
          {"200701020500", false}
          ,{"20000228T051500", false}
          ,{"2007-01-02T05:00:00", false}
          ,{"2000-02-28T05:15:00", false}
          ,{"2000-02-28T05:15:10", false}
          ,{"2000-02-28T05:15:10Z", true}
          ,{"2000-02-28T05:15:10+01:00", true}
          ,{"2000-02-28T05:15:10-02", true}
          ,{"2007-01-02 05:00:00", false}
          ,{"2000-02-28 05:15:00", false}
          ,{"1167714000", true}
      };

  for (const auto& item : data)
  {
      if (item.expected ^ TimeParser::looks_utc(item.src))
      {
          TEST_FAILED((item.src + " should look like "
                  + (item.expected ? "UTC" : "local")
                  + " time").c_str());
      }
  }

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void parse_wintertime()
{
  using namespace Fmi;
  using namespace boost::posix_time;
  using namespace boost::local_time;
  using namespace boost::gregorian;

  auto zone = TimeZoneFactory::instance().time_zone_from_string("Europe/Helsinki");

  local_date_time ok = local_date_time(
      date(2012, 10, 27), hours(3) + minutes(30), zone, local_date_time::EXCEPTION_ON_ERROR);
  local_date_time res = TimeParser::parse("201210270330", zone);
  if (res != ok)
    TEST_FAILED("Expected " + tostring(ok) + ", got " + tostring(res));

  ok = local_date_time(
      date(2012, 10, 27), hours(2) + minutes(30), zone, local_date_time::EXCEPTION_ON_ERROR);
  res = TimeParser::parse("201210270230", zone);
  if (res != ok)
    TEST_FAILED("Expected " + tostring(ok) + ", got " + tostring(res));

  ok = local_date_time(
      date(2012, 10, 27), hours(1) + minutes(30), zone, local_date_time::EXCEPTION_ON_ERROR);
  res = TimeParser::parse("201210270130", zone);
  if (res != ok)
    TEST_FAILED("Expected " + tostring(ok) + ", got " + tostring(res));

  TEST_PASSED();
}

void parse_summertime()
{
  using namespace Fmi;
  using namespace boost::posix_time;
  using namespace boost::local_time;
  using namespace boost::gregorian;

  auto zone = TimeZoneFactory::instance().time_zone_from_string("Europe/Helsinki");

  local_date_time ok = local_date_time(
      date(2013, 3, 31), hours(1) + minutes(30), zone, local_date_time::EXCEPTION_ON_ERROR);
  local_date_time res = TimeParser::parse("201303310130", zone);
  if (res != ok)
    TEST_FAILED("Expected " + tostring(ok) + ", got " + tostring(res));

  ok = local_date_time(
      date(2013, 3, 31), hours(2) + minutes(30), zone, local_date_time::EXCEPTION_ON_ERROR);
  res = TimeParser::parse("201303310230", zone);
  if (res != ok)
    TEST_FAILED("Expected " + tostring(ok) + ", got " + tostring(res));

  // We expect 04:30 for invalid 03:30!!
  ok = local_date_time(
      date(2013, 3, 31), hours(4) + minutes(30), zone, local_date_time::EXCEPTION_ON_ERROR);
  res = TimeParser::parse("201303310330", zone);
  if (res != ok)
    TEST_FAILED("Expected " + tostring(ok) + ", got " + tostring(res));

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
    TEST(parse_wintertime);
    TEST(parse_summertime);
    TEST(parse_timestamp);
    TEST(try_parse_iso);
    TEST(parse_iso);
    TEST(parse_fmi);
    TEST(parse_sql);
    TEST(parse_epoch);
    TEST(parse);
    TEST(parse_http);
    TEST(try_parse_offset);
    TEST(parse_offset);
    TEST(looks);
    TEST(looks_utc);
    TEST(try_parse_duration);
    TEST(parse_duration);
    TEST(parse_iso_duration);
  }
};

}  // namespace TimeParserTest

//! The main program
int main(void)
{
  using namespace std;
  cout << endl << "TimeParser tester" << endl << "=================" << endl;
  TimeParserTest::tests t;
  return t.run();
}

// ======================================================================
