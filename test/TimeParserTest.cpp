// ======================================================================
/*!
 * \file
 * \brief Regression tests for class TimeParser
 */
// ======================================================================

#include "TimeParser.h"
#include "TimeZoneFactory.h"
#include <regression/tframe.h>

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

  ptime res, ok;

  ok = ptime(date(2007, 1, 2), hours(5) + minutes(0));
  if ((res = TimeParser::parse_iso("20070102T0500")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2000, 2, 28), hours(5) + minutes(15));
  if ((res = TimeParser::parse_iso("20000228T0515")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  try
  {
    res = TimeParser::parse_iso("foobar");
    TEST_FAILED("Should fail to parse 'foobar', got " + to_simple_string(res));
  }
  catch (...)
  {
  }

  try
  {
    res = TimeParser::parse_iso("12345678901");
    TEST_FAILED("Should fail to parse '12345678901', got " + to_simple_string(res));
  }
  catch (...)
  {
  }

  try
  {
    res = TimeParser::parse_iso("123456789012");
    TEST_FAILED("Should fail to parse '123456789012', got " + to_simple_string(res));
  }
  catch (...)
  {
  }

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void parse_epoch()
{
  using namespace Fmi;
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  ptime res, ok;

  // date +%s --date="2007-01-02 05:00:00 UTC"  --> 1167714000
  ok = ptime(date(2007, 1, 2), hours(5) + minutes(0));
  if ((res = TimeParser::parse_epoch("1167714000")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  // date +%s --date="2000-02-28 05:15:00 UTC" --> 951714900
  ok = ptime(date(2000, 2, 28), hours(5) + minutes(15));
  if ((res = TimeParser::parse_epoch("951714900")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  try
  {
    res = TimeParser::parse_epoch("foobar");
    TEST_FAILED("Should fail to parse 'foobar', got " + to_simple_string(res));
  }
  catch (...)
  {
  }

  try
  {
    res = TimeParser::parse_epoch("12345678901");
    TEST_FAILED("Should fail to parse '12345678901', got " + to_simple_string(res));
  }
  catch (...)
  {
  }

  // This looks like a valid datetime:

  ok = ptime(date(2014, 05, 13), hours(20) + minutes(8));
  if ((res = TimeParser::parse("1400011680")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void parse_iso()
{
  using namespace Fmi;
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  ptime res, ok;

  ok = ptime(date(2007, 1, 2), hours(5) + minutes(0));
  if ((res = TimeParser::parse_iso("20070102T050000")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2000, 2, 28), hours(5) + minutes(15));
  if ((res = TimeParser::parse_iso("20000228T051500")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2007, 1, 2), hours(5) + minutes(0));
  if ((res = TimeParser::parse_iso("2007-01-02T05:00:00")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2000, 2, 28), hours(5) + minutes(15));
  if ((res = TimeParser::parse_iso("2000-02-28T05:15:00")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2000, 2, 28), hours(5) + minutes(15) + seconds(10));
  if ((res = TimeParser::parse_iso("2000-02-28T05:15:10")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2000, 2, 28), hours(5) + minutes(15) + seconds(10));
  if ((res = TimeParser::parse_iso("2000-02-28T05:15:10Z")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2000, 2, 28), hours(4) + minutes(15) + seconds(10));
  if ((res = TimeParser::parse_iso("2000-02-28T05:15:10+01:00")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2000, 2, 28), hours(3) + minutes(45) + seconds(10));
  if ((res = TimeParser::parse_iso("2000-02-28T05:15:10+01:30")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2000, 2, 28), hours(3) + minutes(45) + seconds(10));
  if ((res = TimeParser::parse_iso("2000-02-28T02:15:10-01:30")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2000, 2, 28), hours(5) + minutes(15) + seconds(10));
  if ((res = TimeParser::parse_iso("2000-02-28T051510")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2000, 2, 28), hours(5) + minutes(15) + seconds(10));
  if ((res = TimeParser::parse_iso("20000228T05:15:10")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2000, 2, 28), hours(5) + minutes(15) + seconds(10));
  if ((res = TimeParser::parse_iso("2000-0228T0515:10")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2000, 2, 28));
  if ((res = TimeParser::parse_iso("20000228T")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2000, 2, 28), hours(12));
  if ((res = TimeParser::parse_iso("20000228T12")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  // Fractional seconds are ignored
  ok = ptime(date(2000, 2, 28), hours(12));
  if ((res = TimeParser::parse_iso("2000-02-28T12:00:00.321")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2000, 2, 28), hours(9));
  if ((res = TimeParser::parse_iso("2000-02-28T12:00:00.321+03:00")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  // Fractional seconds are ignored
  ok = ptime(date(2000, 2, 28), hours(12));
  try
  {
    res = TimeParser::parse_iso("2000-02-28T12:00:00.3233");
    TEST_FAILED("Expected to fail when given four fractional seconds");

    res = TimeParser::parse_iso("2000-02-28T12:00:00.33");
    TEST_FAILED("Expected to fail when given two fractional seconds");

    res = TimeParser::parse_iso("2000-02-28T12:00:00.33");
    TEST_FAILED("Expected to fail when given one fractional second");
  }
  catch (...)
  {
  }

  // BRAINSTORM-480
  ok = ptime(date(2015, 6, 9), hours(13));
  if ((res = TimeParser::parse_iso("2015-06-09T16:00:00+03")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));
  if ((res = TimeParser::parse_iso("2015-06-09T16:00:00+03:00")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));
  if ((res = TimeParser::parse_iso("2015-06-09T16:00:00+0300")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2015, 6, 9), hours(13) + minutes(30));
  if ((res = TimeParser::parse_iso("2015-06-09T08:00:00-05:30")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  if ((res = TimeParser::parse_iso("2015-06-09T08:00:00-0530")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  try
  {
    res = TimeParser::parse_iso("2015-06-09T16:00:00+00300");
    TEST_FAILED("Should fail to parse '00300' timezone, got " + to_simple_string(res));
  }
  catch (...)
  {
  }

  try
  {
    res = TimeParser::parse_iso("2015-06-09T16:00:00--0300");
    TEST_FAILED("Should fail to parse double minus in timezone, got " + to_simple_string(res));
  }
  catch (...)
  {
  }

  try
  {
    res = TimeParser::parse_iso("2015-06-09T16:00:00%0300");
    TEST_FAILED("Should fail to parse invalid sign in timezone, got " + to_simple_string(res));
  }
  catch (...)
  {
  }

#ifdef WE_DO_NOT_SUPPORT_FRACTIONS
  ok = ptime(date(2000, 2, 28), hours(5) + minutes(15) + seconds(10) + microseconds(123456));
  if ((res = TimeParser::parse_iso("2000-02-28T05:15:10.123456")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2000, 2, 28), hours(5) + minutes(15) + seconds(10) + microseconds(987654));
  if ((res = TimeParser::parse_iso("2000-02-28T05:15:10.987654Z")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));
#endif
  // the Gregorian calendar is officially introduced on 1582-10-15
  ok = ptime(date(1582, 10, 15), hours(0) + minutes(0) + seconds(0));
  if ((res = TimeParser::parse_iso("1582-10-15T00:00:00Z")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  try
  {
    res = TimeParser::parse_iso("foobar");
    TEST_FAILED("Should fail to parse 'foobar', got " + to_simple_string(res));
  }
  catch (...)
  {
  }

  try
  {
    res = TimeParser::parse_iso("12345678901");
    TEST_FAILED("Should fail to parse '12345678901', got " + to_simple_string(res));
  }
  catch (...)
  {
  }

  try
  {
    res = TimeParser::parse_iso("foobar");
    TEST_FAILED("Should fail to parse 'foobar', got " + to_simple_string(res));
  }
  catch (...)
  {
  }

  try
  {
    res = TimeParser::parse_iso("12345678901");
    TEST_FAILED("Should fail to parse '12345678901', got " + to_simple_string(res));
  }
  catch (...)
  {
  }

  try
  {
    res = TimeParser::parse_iso("123456789012");
    TEST_FAILED("Should fail to parse '123456789012', got " + to_simple_string(res));
  }
  catch (...)
  {
  }

  // BRAINSTORM-696

  try
  {
    res = TimeParser::parse_iso("20160101T00000");  // extra zero
    TEST_FAILED("Should fail to parse 20160101T00000 due to an extra zero, got " +
                to_simple_string(res));
  }
  catch (...)
  {
  }

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void parse_fmi()
{
  using namespace Fmi;
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  ptime res, ok;

  ok = ptime(date(2009, 12, 12), hours(5) + minutes(0));
  if ((res = TimeParser::parse_fmi("200912120500")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(1800, 1, 1), hours(15) + minutes(0));
  if ((res = TimeParser::parse_fmi("180001011500")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  try
  {
    res = TimeParser::parse_fmi("2009121205");
    TEST_FAILED("Should fail to parse '2009121205', due to missing minute definition");
  }
  catch (...)
  {
  }

  try
  {
    res = TimeParser::parse_fmi("200912120500334");
    TEST_FAILED("Should fail to parse '200912120500334', due to extra tokens");
  }
  catch (...)
  {
  }

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void try_parse_iso()
{
  using namespace Fmi;
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  bool utc;
  ptime res, ok;

  ok = ptime(date(2007, 1, 2), hours(5) + minutes(0));
  if ((res = TimeParser::try_parse_iso("20070102T050000", &utc)) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));
  if (utc) TEST_FAILED("Expected 20070102T050000 to be flagged as local time");

  ok = ptime(date(2000, 2, 28), hours(5) + minutes(15));
  if ((res = TimeParser::try_parse_iso("20000228T051500", &utc)) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));
  if (utc) TEST_FAILED("Expected 20000228T051500 to be flagged as local time");

  ok = ptime(date(2000, 2, 28), hours(5) + minutes(15) + seconds(10));
  if ((res = TimeParser::try_parse_iso("2000-02-28T05:15:10Z", &utc)) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));
  if (!utc) TEST_FAILED("Expected 2000-02-28T05:15:10Z to be flagged as UTC");

  ok = ptime(date(2000, 2, 28), hours(4) + minutes(15) + seconds(10));
  if ((res = TimeParser::try_parse_iso("2000-02-28T05:15:10+01:00", &utc)) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));
  if (!utc) TEST_FAILED("Expected 2000-02-28T05:15:10+01:00 to be flagged as UTC");

  ok = ptime(date(2000, 2, 28), hours(6) + minutes(45) + seconds(10));
  if ((res = TimeParser::try_parse_iso("2000-02-28T05:15:10-01:30", &utc)) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));
  if (!utc) TEST_FAILED("Expected 2000-02-28T05:15:10-01:30 to be flagged as UTC");

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void parse_sql()
{
  using namespace Fmi;
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  ptime res, ok;

  ok = ptime(date(2007, 1, 2), hours(5) + minutes(0));
  if ((res = TimeParser::parse_sql("2007-01-02 05:00:00")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2000, 2, 28), hours(5) + minutes(15));
  if ((res = TimeParser::parse_sql("2000-02-28 05:15:00")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2000, 2, 28), hours(5));
  if ((res = TimeParser::parse_sql("2000-02-28 05")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2000, 2, 28), hours(5) + minutes(20) + seconds(34));
  if ((res = TimeParser::parse_sql("2000-02-28 05:20:34")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  ok = ptime(date(2000, 2, 28));
  if ((res = TimeParser::parse_sql("2000-02-28")) != ok)
    TEST_FAILED("Expected " + to_simple_string(ok) + ", got " + to_simple_string(res));

  try
  {
    res = TimeParser::parse_sql("foobar");
    TEST_FAILED("Should fail to parse 'foobar', got " + to_simple_string(res));
  }
  catch (...)
  {
  }

  try
  {
    res = TimeParser::parse_sql("12345678901");
    TEST_FAILED("Should fail to parse '12345678901', got " + to_simple_string(res));
  }
  catch (...)
  {
  }

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

  ptime res, ok;

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

void parse_duration()
{
  using namespace Fmi;
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  time_duration res, ok;

  res = TimeParser::parse_duration("0");
  if (res != seconds(0)) TEST_FAILED("Failed to parse 0 correctly, got " + to_simple_string(res));

  res = TimeParser::parse_duration("+1");
  if (res != minutes(1)) TEST_FAILED("Failed to parse +1 correctly, got " + to_simple_string(res));

  res = TimeParser::parse_duration("+10m");
  if (res != minutes(10))
    TEST_FAILED("Failed to parse +10m correctly, got " + to_simple_string(res));

  res = TimeParser::parse_duration("+3h");
  if (res != hours(3)) TEST_FAILED("Failed to parse +3h correctly, got " + to_simple_string(res));

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
  if (res != hours(24)) TEST_FAILED("Failed to parse P1D correctly, got " + to_simple_string(res));

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
  if (res != hours(3)) TEST_FAILED("Failed to parse PT3H correctly, got " + to_simple_string(res));

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

  if ((res = TimeParser::looks("2007-01-02 05:00:00")) != "sql")
    TEST_FAILED("2007-01-02 05:00:00 should look like a SQL date time");

  if ((res = TimeParser::looks("2000-02-28 05:15:00")) != "sql")
    TEST_FAILED("2000-02-28 05:15:00 should look like a SQL date time");

  if ((res = TimeParser::looks("1167714000")) != "epoch")
    TEST_FAILED("1167714000 should look like an epoch time");

  if ((res = TimeParser::looks("1357063111")) != "epoch")
    TEST_FAILED("1357063111 should look like an epoch time");

  if ((res = TimeParser::looks("+10")) != "offset") TEST_FAILED("+10 should look like an offset");
  if ((res = TimeParser::looks("-100")) != "offset") TEST_FAILED("-100 should look like an offset");
  if ((res = TimeParser::looks("+10m")) != "offset") TEST_FAILED("+10m should look like an offset");
  if ((res = TimeParser::looks("-100H")) != "offset")
    TEST_FAILED("-100H should look like an offset");

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

  if (TimeParser::looks_utc("200701020500"))
    TEST_FAILED("200701020500 should look like local time");

  if (TimeParser::looks_utc("20000228T051500"))
    TEST_FAILED("20000228T051500 should look like local time");

  if (TimeParser::looks_utc("2007-01-02T05:00:00"))
    TEST_FAILED("2007-01-02T05:00:00 should look like local time");

  if (TimeParser::looks_utc("2000-02-28T05:15:00"))
    TEST_FAILED("2000-02-28T05:15:00 should look like local time");

  if (TimeParser::looks_utc("2000-02-28T05:15:10"))
    TEST_FAILED("2000-02-28T05:15:10 should look like local time");

  if (!TimeParser::looks_utc("2000-02-28T05:15:10Z"))
    TEST_FAILED("2000-02-28T05:15:10Z should look like UTC time");

  if (!TimeParser::looks_utc("2000-02-28T05:15:10+01:00"))
    TEST_FAILED("2000-02-28T05:15:10Z should look like UTC time");

  if (!TimeParser::looks_utc("2000-02-28T05:15:10-02"))
    TEST_FAILED("2000-02-28T05:15:10Z should look like UTC time");

  if (TimeParser::looks_utc("2007-01-02 05:00:00"))
    TEST_FAILED("2007-01-02 05:00:00 should look like local time");

  if (TimeParser::looks_utc("2000-02-28 05:15:00"))
    TEST_FAILED("2000-02-28 05:15:00 should look like local time");

  if (!TimeParser::looks_utc("1167714000")) TEST_FAILED("1167714000 should look like UTC time");

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
  if (res != ok) TEST_FAILED("Expected " + tostring(ok) + ", got " + tostring(res));

  ok = local_date_time(
      date(2012, 10, 27), hours(2) + minutes(30), zone, local_date_time::EXCEPTION_ON_ERROR);
  res = TimeParser::parse("201210270230", zone);
  if (res != ok) TEST_FAILED("Expected " + tostring(ok) + ", got " + tostring(res));

  ok = local_date_time(
      date(2012, 10, 27), hours(1) + minutes(30), zone, local_date_time::EXCEPTION_ON_ERROR);
  res = TimeParser::parse("201210270130", zone);
  if (res != ok) TEST_FAILED("Expected " + tostring(ok) + ", got " + tostring(res));

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
  if (res != ok) TEST_FAILED("Expected " + tostring(ok) + ", got " + tostring(res));

  ok = local_date_time(
      date(2013, 3, 31), hours(2) + minutes(30), zone, local_date_time::EXCEPTION_ON_ERROR);
  res = TimeParser::parse("201303310230", zone);
  if (res != ok) TEST_FAILED("Expected " + tostring(ok) + ", got " + tostring(res));

  // We expect 04:30 for invalid 03:30!!
  ok = local_date_time(
      date(2013, 3, 31), hours(4) + minutes(30), zone, local_date_time::EXCEPTION_ON_ERROR);
  res = TimeParser::parse("201303310330", zone);
  if (res != ok) TEST_FAILED("Expected " + tostring(ok) + ", got " + tostring(res));

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
    TEST(parse_iso);
    TEST(parse_fmi);
    TEST(try_parse_iso);
    TEST(parse_sql);
    TEST(parse_epoch);
    TEST(parse);
    TEST(parse_http);
    TEST(parse_offset);
    TEST(looks);
    TEST(looks_utc);
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
