#pragma once

//#define MYDEBUG

#ifdef MYDEBUG
#define BOOST_SPIRIT_DEBUG
#endif

#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/optional.hpp>

#include <string>

namespace Fmi
{
namespace TimeParser
{
enum ParserId
{

  SQL,
  ISO,
  EPOCH,
  OFFSET,
  FMI

};

typedef boost::optional<unsigned int> OptionalInt;
typedef boost::optional<std::string> OptionalString;
typedef boost::optional<char> OptionalChar;

struct TimeZoneOffset
{
  char sign;

  unsigned int hours;

  unsigned int minutes;
};

typedef unsigned int UnixTime;

struct TimeOffset
{
  char sign;

  unsigned int value;

  OptionalChar unit;
};

struct TimeStamp
{
  unsigned short year;

  unsigned short month;

  unsigned short day;

  OptionalInt hour;

  OptionalInt minute;

  OptionalInt second;

  TimeZoneOffset tz;
};
}
}

// Adapt structs, so they behave like boost::fusion::vector's

BOOST_FUSION_ADAPT_STRUCT(Fmi::TimeParser::TimeZoneOffset,
                          (char, sign)(unsigned int, hours)(unsigned int, minutes))

BOOST_FUSION_ADAPT_STRUCT(Fmi::TimeParser::TimeOffset,
                          (char, sign)(unsigned int, value)(Fmi::TimeParser::OptionalChar, unit))

BOOST_FUSION_ADAPT_STRUCT(Fmi::TimeParser::TimeStamp,
                          (unsigned short, year)(unsigned short, month)(unsigned short, day)(
                              Fmi::TimeParser::OptionalInt,
                              hour)(Fmi::TimeParser::OptionalInt,
                                    minute)(Fmi::TimeParser::OptionalInt,
                                            second)(Fmi::TimeParser::TimeZoneOffset, tz))

namespace Fmi
{
namespace TimeParser
{
namespace spirit = boost::spirit;
namespace qi = boost::spirit::qi;

//########################################################
// Timezone parser
//########################################################

template <typename Iterator>
struct TimeZoneParser : qi::grammar<Iterator, TimeZoneOffset()>
{
  typedef qi::uint_parser<unsigned int, 10, 2, 2> TwoDigitNumber;  // Radix 10, exactly 2 digits

  TimeZoneParser() : TimeZoneParser::base_type(tz_offset)
  {
    tz_offset =
        (qi::lit('Z') >> qi::attr('+') >> qi::attr(0) >> qi::attr(0))  // Z means zero offset
        |
        ((qi::char_("+") | qi::char_("-")) >>
         (two_digits >>
          (two_digits | ((qi::lit(':') >> two_digits) |
                         qi::attr(0)))))  // sign with hours followed by optional colon and minutes
        | (qi::attr('+') >> qi::attr(0) >>
           qi::attr(0));  // If parse failed, default to 0 hours 0 minutes

#ifdef MYDEBUG
    BOOST_SPIRIT_DEBUG_NODE(two_digits);
    BOOST_SPIRIT_DEBUG_NODE(tz_offset);
#endif
  }

  qi::rule<Iterator, TimeZoneOffset()> tz_offset;
  TwoDigitNumber two_digits;
};

//########################################################
// SQL-style timestring parser
//########################################################

template <typename Iterator>
struct SQLParser : qi::grammar<Iterator, TimeStamp()>
{
  SQLParser() : SQLParser::base_type(fulltime)
  {
    number = qi::uint_;

    fulltime = number >> '-'                   // Year
               >> number                       // Month
               >> '-' >> number                // Day
               >> qi::omit[*qi::ascii::blank]  // Any number of blanks
               >> -number                      // Optional hour
               >> -(':' >> number)             // Optional minute
               >> -(':' >> number)             // Optional second
               >> qi::eoi;                     // End-of-input

#ifdef MYDEBUG
    BOOST_SPIRIT_DEBUG_NODE(number);
    BOOST_SPIRIT_DEBUG_NODE(fulltime);
#endif
  }

  qi::rule<Iterator, unsigned int()> number;
  qi::rule<Iterator, TimeStamp()> fulltime;
};

//########################################################
// FMI-style timestring parser
// YYYYMMDDHHmm - all mandatory
//########################################################

template <typename Iterator>
struct FMIParser : qi::grammar<Iterator, TimeStamp()>
{
  typedef qi::uint_parser<unsigned int, 10, 4, 4> FourDigitNumber;  // Radix 10, exactly 4 digits
  typedef qi::uint_parser<unsigned int, 10, 2, 2> TwoDigitNumber;   // Radix 10, exactly 2 digits

  FMIParser() : FMIParser::base_type(fmitime)
  {
    fmitime = four_digits     // Year
              >> two_digits   // Month
              >> two_digits   // Day
              >> two_digits   // Hhour
              >> two_digits   // Minute
              >> -two_digits  // Second
              >> offset       // We support timezone-offsets, just because
              >> qi::eoi;     // End-of-input

#ifdef MYDEBUG
    BOOST_SPIRIT_DEBUG_NODE(fmitime);
#endif
  }

  FourDigitNumber four_digits;
  TwoDigitNumber two_digits;

  TimeZoneParser<Iterator> offset;

  qi::rule<Iterator, TimeStamp()> fmitime;
};

//########################################################
// Epoch parser
//########################################################

typedef qi::uint_parser<UnixTime, 10, 5, 11> EpochParser;  // Radix 10, min digits 5, max digits 11

//########################################################
// Offset-style timestring parser
//########################################################

template <typename Iterator>
struct OffsetParser : qi::grammar<Iterator, TimeOffset()>
{
  OffsetParser() : OffsetParser::base_type(complete_offset)
  {
    number = qi::uint_;  // A simple integer

    optional_character = -qi::char_;  // Optional single character

    offset = (qi::char_('+') | qi::char_('-'))  // Preceeding plus or minus sign
             >> number                          // an unsigned number
             >> optional_character              // optional character for unit
             >> qi::eoi;                        // end-of-input

    zero_offset = qi::lit("0") >> qi::eoi >> qi::attr('+') >> qi::attr(0) >>
                  qi::attr('m');  // Special case when input is plain 0
    zero_unit_offset = qi::lit("0") >> qi::attr('+') >> qi::attr(0) >> optional_character >>
                       qi::eoi;  // Special case when input is plain 0 with units

    complete_offset = zero_offset | zero_unit_offset | offset;

#ifdef MYDEBUG
    BOOST_SPIRIT_DEBUG_NODE(number);
    BOOST_SPIRIT_DEBUG_NODE(optional_character);
    BOOST_SPIRIT_DEBUG_NODE(offset);
    BOOST_SPIRIT_DEBUG_NODE(zero_offset);
    BOOST_SPIRIT_DEBUG_NODE(complete_offset);
#endif
  }

  qi::rule<Iterator, unsigned int()> number;
  qi::rule<Iterator, OptionalChar()> optional_character;
  qi::rule<Iterator, TimeOffset()> zero_offset;
  qi::rule<Iterator, TimeOffset()> zero_unit_offset;
  qi::rule<Iterator, TimeOffset()> offset;
  qi::rule<Iterator, TimeOffset()> complete_offset;
};

//########################################################
// ISO-style timestring parser
//
// This parses time strings in the form of:
// YYYY(sep)MM(sep)DDTHH(sep)MMSS.sss<timezone>
//
// Separator T is mandatory if sub-daily precision is
// required
//
// Fractional seconds are parsed but ignored due to being irrelevant
//########################################################

template <typename Iterator>
struct ISOParser : qi::grammar<Iterator, TimeStamp()>
{
  typedef qi::uint_parser<unsigned int, 10, 4, 4> FourDigitNumber;   // Radix 10, exactly 4 digits
  typedef qi::uint_parser<unsigned int, 10, 3, 3> ThreeDigitNumber;  // Radix 10, exactly 4 digits
  typedef qi::uint_parser<unsigned int, 10, 2, 2> TwoDigitNumber;    // Radix 10, exactly 2 digits

  ISOParser() : ISOParser::base_type(isostamp)
  {
    optional_dash = qi::omit[-qi::lit('-')];

    optional_colon = qi::omit[-qi::lit(':')];

    optional_period = qi::omit[-qi::lit('.')];

    date_time_separator = qi::omit[qi::lit('T')];

    isostamp = four_digits >> optional_dash >> two_digits >> optional_dash >>
               two_digits              // Digits optionally separated by dashes
               >> date_time_separator  // An optional literal 'T'
               >> -two_digits >> optional_colon >> -two_digits >> optional_colon >>
               -two_digits  // Three  optional sets of two digits, optionally separated by colons
               >> optional_period >> -qi::omit[three_digits] >>
               tz_parser    // Timezone, defaults to 0 as per the parser
               >> qi::eoi;  // End of input

#ifdef MYDEBUG
    BOOST_SPIRIT_DEBUG_NODE(dash);
    BOOST_SPIRIT_DEBUG_NODE(colon);
    BOOST_SPIRIT_DEBUG_NODE(isostamp);
#endif
  }

  FourDigitNumber four_digits;
  ThreeDigitNumber three_digits;
  TwoDigitNumber two_digits;

  qi::rule<Iterator, void()> optional_dash;
  qi::rule<Iterator, void()> optional_colon;
  qi::rule<Iterator, void()> optional_period;
  qi::rule<Iterator, void()> date_time_separator;
  TimeZoneParser<Iterator> tz_parser;

  qi::rule<Iterator, TimeStamp()> isostamp;
};
}
}
