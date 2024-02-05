#pragma once

#include <ctime>
#include "Base.h"

namespace Fmi
{
    namespace date_time
    {
        struct YMD
        {
            int year;
            unsigned month;
            unsigned day;
        };

        class Date : public Base
        {
        public:
            Date();
            Date(const Type& type);
            Date(const DateTimeNS::local_days& date);
            Date(const Date& other);
            Date(int year, int month, int day);
            virtual ~Date();

            Date& operator=(const Date& other);

            int year() const;
            unsigned month() const;
            unsigned day() const;
            YMD year_month_day() const;
            Weekday day_of_week() const;
            int day_of_year() const;
            Date end_of_month() const;
            long modjulian_day() const;
            long julian_day() const;

            /**
             * @return ISO 8601 week number
             */
            int week_number() const;

            std::string as_string() const;
            std::string as_iso_string() const;
            std::string as_iso_extended_string() const;

            bool operator==(const Date& other) const;
            bool operator!=(const Date& other) const;
            bool operator<(const Date& other) const;
            bool operator<=(const Date& other) const;
            bool operator>(const Date& other) const;
            bool operator>=(const Date& other) const;

            Date operator+(int num_days) const;
            Date operator-(int num_days) const;
            int operator-(const Date& other) const;

            Date& operator+=(int num_days);
            Date& operator-=(int num_days);

            const DateTimeNS::local_days& get_impl() const { return date; };

            /**
             * @brief  Parse time duration from string.
             * 
             * @param str  String to parse.
             * @param assume_eoi  If true, allows only whitespace after the duration.
             * @return Parsed date.
            */
            static Date from_stream(std::istream& src, bool assume_eoi = true);

        private:
            void assert_special() const;

        private:
            DateTimeNS::local_days date;
        };

        Date date_from_string(const std::string& str);

        std::string to_simple_string(const Date& date);
        std::string to_iso_string(const Date& date);
        std::string to_iso_extended_string(const Date& date);

        std::ostream& operator<<(std::ostream& os, const Date& date);

        struct tm to_tm(const Date& date);
        Date from_tm(const struct tm& tm);
    }
}