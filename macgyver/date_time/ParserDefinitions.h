#pragma once

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/optional.hpp>
#include <boost/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>

#include <cstdlib>
#include <limits>
#include <string>

#include "ParserStructures.h"

// Adapt structs, so they behave like boost::fusion::vector's

BOOST_FUSION_ADAPT_STRUCT(
    Fmi::date_time::parser::date_members_t,
    (unsigned, year)
    (unsigned, month)
    (unsigned, mday))

BOOST_FUSION_ADAPT_STRUCT(
    Fmi::date_time::parser::seconds_members_t,
    (unsigned, seconds)
    (std::string, frac_sec))

BOOST_FUSION_ADAPT_STRUCT(
    Fmi::date_time::parser::duration_members_t,
    (unsigned, hours)
    (unsigned, minutes)
    (std::optional<Fmi::date_time::parser::seconds_members_t>, seconds))

BOOST_FUSION_ADAPT_STRUCT(
    Fmi::date_time::parser::time_zone_offset_members_t,
    (char, sign)
    (unsigned, hours)
    (unsigned, minutes))

BOOST_FUSION_ADAPT_STRUCT(
    Fmi::date_time::parser::date_time_members_t,
    (Fmi::date_time::parser::date_members_t, date)
    (std::optional<Fmi::date_time::parser::duration_members_t>, time)
    (std::optional<Fmi::date_time::parser::time_zone_offset_members_t>, tz_offset))

namespace Fmi
{
namespace date_time
{

namespace parser
{
    namespace qi = boost::spirit::qi;

    using r_uint12 = qi::uint_parser<unsigned, 10, 1, 2>;
    using r_uint22 = qi::uint_parser<unsigned, 10, 2, 2>;
    using r_uint14 = qi::uint_parser<unsigned, 10, 2, 4>;
    using r_uint44 = qi::uint_parser<unsigned, 10, 4, 4>;

    template <typename Iterator>
    class MonthShortNameParser : public qi::grammar<Iterator, unsigned()>
    {
    public:
        MonthShortNameParser()
            : MonthShortNameParser::base_type(m_month)
        {
            using namespace qi;
            m_month = (no_case[lit("Jan")] >> attr(1)) |
                        (no_case[lit("Feb")] >> attr(2)) |
                        (no_case[lit("Mar")] >> attr(3)) |
                        (no_case[lit("Apr")] >> attr(4)) |
                        (no_case[lit("May")] >> attr(5)) |
                        (no_case[lit("Jun")] >> attr(6)) |
                        (no_case[lit("Jul")] >> attr(7)) |
                        (no_case[lit("Aug")] >> attr(8)) |
                        (no_case[lit("Sep")] >> attr(9)) |
                        (no_case[lit("Oct")] >> attr(10)) |
                        (no_case[lit("Nov")] >> attr(11)) |
                        (no_case[lit("Dec")] >> attr(12));
        }

    private:
        qi::rule<Iterator, unsigned()> m_month;
    };

    template <typename Iterator>
    struct DateParser : public qi::grammar<Iterator, date_members_t()>
    {
        DateParser(char separator, bool numerical_month)
            : DateParser::base_type(m_date)
        {
            using namespace qi;
            if (separator == 0)
            {
                m_year %= r_uint44()[_pass = (_1 >= 0 && _1 <= 9999)];
                m_mday %= r_uint22()[_pass = (_1 >= 1 && _1 <= 31)];
                if (numerical_month)
                {
                    m_month %= r_uint22()[_pass = (_1 >= 1 && _1 <= 12)];
                }
                else
                {
                    m_month%= m_month_abbrev;
                }
                m_date %= lexeme[m_year >> m_month >> m_mday];
            }
            else
            {
                m_year %= r_uint14()[_pass = (_1 >= 0 && _1 <= 9999)];
                m_mday %= r_uint12()[_pass = (_1 >= 1 && _1 <= 31)];
                if (numerical_month)
                {
                    m_month %= r_uint12()[_pass = (_1 >= 1 && _1 <= 12)];
                }
                else
                {
                    m_month %= m_month_abbrev;
                }
                m_date %= lexeme[
                    m_year >>
                    omit[char_(separator)] >>
                    m_month >>
                    omit[char_(separator)] >>
                    m_mday];
            }
        }

    private:
        MonthShortNameParser<Iterator> m_month_abbrev;
        qi::rule<Iterator, unsigned()> m_year;
        qi::rule<Iterator, unsigned()> m_month;
        qi::rule<Iterator, unsigned()> m_mday;
        qi::rule<Iterator, date_members_t()> m_date;
    };

    template <typename Iterator>
    struct SecondsParser : public qi::grammar<Iterator, seconds_members_t()>
    {
        SecondsParser(bool two_digits_required)
            : SecondsParser::base_type(m_seconds)
        {
            using namespace qi;
            if (two_digits_required) {
                m_full_seconds %= r_uint22()[_pass = (_1 >= 0 && _1 <= 59)];
            } else {
                m_full_seconds %= r_uint12()[_pass = (_1 >= 0 && _1 <= 59)];
            }
            m_frac_seconds %= lexeme[omit['.'] >> +digit];
            m_seconds %= m_full_seconds
                >> (m_frac_seconds | (eps >> attr(std::string{})));
        }
    private:
        qi::rule<Iterator, unsigned()> m_full_seconds;
        qi::rule<Iterator, std::string()> m_frac_seconds;
        qi::rule<Iterator, seconds_members_t()> m_seconds;
    };

    template <typename Iterator>
    struct DurationParser : public qi::grammar<Iterator, duration_members_t()>
    {
        using iterator = std::string::const_iterator;

        /**
         * @brief Construct a new Duration Parser object
         * 
         * @param separator separator character between hours, minutes and seconds when not
         *                  0 and absent when 0
        */
        DurationParser(
            const char separator = ':',
            const unsigned max_hours = std::numeric_limits<unsigned>::max())

            : DurationParser::base_type(m_duration)
            , m_seconds(separator == 0)
        {
            using namespace qi;

            if (separator == 0)
            {
                m_hours %= r_uint22()[_pass = (_1 >= 0 && _1 <= max_hours)];
                m_minutes %= r_uint22()[_pass = (_1 >= 0 && _1 <= 59)];
                m_duration %= lexeme[m_hours >> m_minutes >> m_seconds];
            }
            else
            {
                m_hours %= r_uint12()[_pass = (_1 >= 0 && _1 <= max_hours)];
                m_minutes %= r_uint12()[_pass = (_1 >= 0 && _1 <= 59)];
                // Seconds are optional
                m_duration %= lexeme[
                    m_hours
                    >> omit[char_(separator)]
                    >> m_minutes
                    >> -(omit[char_(separator)] >> m_seconds)
                ];
            }
        }

    private:
        qi::rule<Iterator, unsigned()> m_hours;
        qi::rule<Iterator, unsigned()> m_minutes;
        SecondsParser<Iterator> m_seconds;
        qi::rule<Iterator, duration_members_t()> m_duration;
    };

    template <typename Iterator>
    struct TimeZoneOffsetParser : public qi::grammar<Iterator, time_zone_offset_members_t()>
    {
        TimeZoneOffsetParser()
            : TimeZoneOffsetParser::base_type(m_offset)
        {
            using namespace qi;
            m_hours %= r_uint22() [_pass = (_1 >= 0 && _1 <= 14)];
            m_opt_sep %= -omit[char(':')];
            m_minutes %= r_uint22() [_pass = (_1 >= 0 && _1 <= 59)];
            m_sign %= (char_('+') | char_('-'));

            m_utc %= char_('Z') >> attr(0) >> attr(0);
            m_given_offset %= m_sign >> m_hours >> m_opt_sep >> m_minutes;

            m_offset %= lexeme[ m_utc | m_given_offset];
        }

    private:
        qi::rule<Iterator, char()> m_sign;
        qi::rule<Iterator, unsigned()> m_hours;
        qi::rule<Iterator, void()> m_opt_sep;
        qi::rule<Iterator, unsigned()> m_minutes;
        qi::rule<Iterator, time_zone_offset_members_t()> m_utc;
        qi::rule<Iterator, time_zone_offset_members_t()> m_given_offset;
        qi::rule<Iterator, time_zone_offset_members_t()> m_offset;
    };

    template <typename Iterator>
    struct DateTimeParser : public qi::grammar<Iterator, date_time_members_t()>
    {
        DateTimeParser(char date_separator, char time_separator, bool numerical_month)
            : DateTimeParser::base_type(m_date_time)
            , m_date(date_separator, numerical_month)
            , m_duration(time_separator)
            , m_offset()
        {
            using namespace qi;
            m_date_time %= lexeme[m_date >> -('T' >> m_duration >> -m_offset)];
        }

    private:
        DateParser<Iterator> m_date;
        DurationParser<Iterator> m_duration;
        TimeZoneOffsetParser<Iterator> m_offset;
        qi::rule<Iterator, date_time_members_t()> m_date_time;
    };

}  // namespace parser

}  // namespace date_time
}  // namespace Fmi
