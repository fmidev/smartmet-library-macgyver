#include "date_time/ParserDefinitions.h"

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/support_istream_iterator.hpp>
;
#include <boost/test/included/unit_test.hpp>

using namespace boost::unit_test;
using namespace std::string_literals;

test_suite* init_unit_test_suite(int argc, char* argv[])
{
  const char* name = "Fmi::date_time::ParserDefinitions test";
  // unit_test_log.set_threshold_level(log_test_units);
  unit_test_log.set_threshold_level(log_messages);
  framework::master_test_suite().p_name.value = name;
  BOOST_TEST_MESSAGE("");
  BOOST_TEST_MESSAGE(name);
  BOOST_TEST_MESSAGE(std::string(std::strlen(name), '='));
  std::setlocale(LC_ALL, "C");
  return NULL;
}

BOOST_AUTO_TEST_CASE (test_month_short_name_parser)
{
    BOOST_TEST_MESSAGE("Month shory name parser");
    bool ok;
    unsigned result;
    Fmi::date_time::parser::MonthShortNameParser parser;

    std::string input = "Jan";
    std::string::const_iterator begin = input.begin();
    std::string::const_iterator end = input.end();
    ok = boost::spirit::qi::parse(begin, end, parser, result);
    BOOST_CHECK(ok);
    if (ok) {
        BOOST_CHECK_EQUAL(result, 1);
    }

    input = "feb";
    begin = input.begin();
    end = input.end();
    ok = boost::spirit::qi::parse(begin, end, parser, result);
    BOOST_CHECK(ok);
    if (ok) {
        BOOST_CHECK_EQUAL(result, 2);
    }
}

BOOST_AUTO_TEST_CASE(test_parse_date_iso_string)
{
    BOOST_TEST_MESSAGE("Parse ISO string");
    bool ok;
    Fmi::date_time::parser::date_members_t date_result;
    Fmi::date_time::parser::IsoDateParser parser;

    std::string input = "20170314";
    std::string::const_iterator begin = input.begin();
    std::string::const_iterator end = input.end();
    ok = boost::spirit::qi::parse(begin, end, parser, date_result);
    BOOST_CHECK(ok);
    if (ok) {
        BOOST_CHECK_EQUAL(date_result.year, 2017);
        BOOST_CHECK_EQUAL(date_result.month, 3);
        BOOST_CHECK_EQUAL(date_result.mday, 14);
    }

    input = "2017-03-14";
    begin = input.begin();
    end = input.end();
    ok = boost::spirit::qi::parse(begin, end, parser, date_result);
    BOOST_CHECK(!ok);
}

BOOST_AUTO_TEST_CASE(test_parse_date_iso_extended_string)
{
    BOOST_TEST_MESSAGE("Parse ISO string");
    bool ok;
    Fmi::date_time::parser::date_members_t date_result;
    Fmi::date_time::parser::IsoExtendedDateParser parser;

    std::string input = "2017-03-14";
    std::string::const_iterator begin = input.begin();
    std::string::const_iterator end = input.end();
    ok = boost::spirit::qi::parse(begin, end, parser, date_result);
    BOOST_CHECK(ok);
    if (ok) {
        BOOST_CHECK_EQUAL(date_result.year, 2017);
        BOOST_CHECK_EQUAL(date_result.month, 3);
        BOOST_CHECK_EQUAL(date_result.mday, 14);
    }

    input = "20170314";
    begin = input.begin();
    end = input.end();
    ok = boost::spirit::qi::parse(begin, end, parser, date_result);
    BOOST_CHECK(!ok);
}

BOOST_AUTO_TEST_CASE (test_simple_date_parser)
{
    BOOST_TEST_MESSAGE("Simple date parser");

    struct {
        std::string input;
        unsigned year;
        unsigned month;
        unsigned mday;
    }
    const test_data_pass[] = {
        {"2017-03-14"s, 2017, 3, 14},
        {"2017-3-4"s, 2017, 3, 4},
        {"2024-Feb-27"s, 2024, 2, 27}
    };

    const std::string test_input_fail[] = {
        "",
        "2017-03 14",
        "2017-13-14",
        "2017-03-32"
    };

    using iterator = std::string::const_iterator;

    for (const auto& td : test_data_pass) {
        Fmi::date_time::parser::SimpleDateParser parser;
        Fmi::date_time::parser::date_members_t result;
        iterator begin = td.input.begin();
        iterator end = td.input.end();
        bool ok = boost::spirit::qi::parse(begin, end, parser, result);
        if (ok) {
            BOOST_CHECK_EQUAL(result.year, td.year);
            BOOST_CHECK_EQUAL(result.month, td.month);
            BOOST_CHECK_EQUAL(result.mday, td.mday);
        } else {
            BOOST_FAIL("Parsing failed for input: " + td.input);
        }
    }   

    for (const auto& input : test_input_fail) {
        Fmi::date_time::parser::SimpleDateParser parser;
        Fmi::date_time::parser::date_members_t result;
        iterator begin = input.begin();
        iterator end = input.end();
        bool ok = boost::spirit::qi::parse(begin, end, parser, result);
        if (ok) {
            BOOST_FAIL("Parsing should have failed for input: " + input);
        }
    }
}
