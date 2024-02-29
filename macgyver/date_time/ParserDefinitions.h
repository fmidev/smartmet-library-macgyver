#pragma once

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/optional.hpp>
#include <boost/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>

#include <string>

namespace Fmi
{
namespace date_time
{
namespace parser
{
    struct date_members_t
    {
        unsigned year;
        unsigned month;
        unsigned mday;
    };

    struct time_members_t
    {
        unsigned hours;
        unsigned minutes;
        double seconds;
    };

}  // namespace detail
} // namespace date_time
}  // namespace Fmi

// Adapt structs, so they behave like boost::fusion::vector's

BOOST_FUSION_ADAPT_STRUCT(
    Fmi::date_time::parser::date_members_t,
    (unsigned, year)
    (unsigned, month)
    (unsigned, mday))

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
    using r_uint34 = qi::uint_parser<unsigned, 10, 4, 4>;

    class MonthShortNameParser : public qi::grammar<std::string::const_iterator, unsigned()>
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
        qi::rule<std::string::const_iterator, unsigned()> m_month;
    };

    class IsoDateParser : public qi::grammar<std::string::const_iterator, date_members_t()>
    {
        using iterator = std::string::const_iterator;
    public:
        IsoDateParser()
            : IsoDateParser::base_type(m_date)
        {
            using namespace qi;
            m_year %= r_uint14()[_pass = (_1 >= 0 && _1 <= 9999)];
            m_month_num %= r_uint22()[_pass = (_1 >= 1 && _1 <= 12)];
            m_mday %= r_uint22()[_pass = (_1 >= 1 && _1 <= 31)];

            m_date %= lexeme[m_year >> m_month_num >> m_mday];
        }

    private:
        qi::rule<iterator, unsigned()> m_year;
        qi::rule<iterator, unsigned()> m_month_num;
        qi::rule<iterator, unsigned()> m_mday;
        qi::rule<std::string::const_iterator, date_members_t()> m_date;
    };

    class IsoExtendedDateParser : public qi::grammar<std::string::const_iterator, date_members_t()>
    {
        using iterator = std::string::const_iterator;

    public:
        IsoExtendedDateParser()
            : IsoExtendedDateParser::base_type(m_date)
        {
            using namespace qi;
            m_year %= r_uint14()[_pass = (_1 >= 0 && _1 <= 9999)];
            m_month_num %= r_uint22()[_pass = (_1 >= 1 && _1 <= 12)];
            m_mday %= r_uint22()[_pass = (_1 >= 1 && _1 <= 31)];

            m_date %= lexeme[m_year >> '-' >> m_month_num >> '-' >> m_mday];
        }

    private:
        qi::rule<iterator, unsigned()> m_year;
        qi::rule<iterator, unsigned()> m_month_num;
        qi::rule<iterator, unsigned()> m_mday;
        qi::rule<std::string::const_iterator, date_members_t()> m_date;
    };

    class SimpleDateParser : public qi::grammar<std::string::const_iterator, date_members_t()>
    {
        using iterator = std::string::const_iterator;
    public:
        SimpleDateParser()
            : SimpleDateParser::base_type(m_date)
        {
            using namespace qi;
            m_year %= r_uint14()[_pass = (_1 >= 0 && _1 <= 9999)];
            m_month_num %= r_uint12()[_pass = (_1 >= 1 && _1 <= 12)];
            m_mday %= r_uint12()[_pass = (_1 >= 1 && _1 <= 31)];

            m_date = lexeme[
                m_year >> omit['-']
                >> (m_month_num | m_month_abbrev)
                >> omit['-'] >> m_mday];
        }

    private:
        qi::rule<iterator, unsigned()> m_year;
        qi::rule<iterator, unsigned()> m_month_num;
        MonthShortNameParser m_month_abbrev;
        qi::rule<iterator, unsigned()> m_mday;
        qi::rule<std::string::const_iterator, date_members_t()> m_date;
    };

}  // namespace detail

}  // namespace date_time
}  // namespace Fmi
