#pragma once

#include "Date.h"
#include "TimeDuration.h"
#include <ctime>
#include <ostream>

namespace Fmi
{
    namespace date_time
    {
        class DateTime : public Base
        {
        public:
            DateTime();
            DateTime(const Type& type);
            DateTime(const DateTime& other);
            DateTime(const Date& date);
            DateTime(const Date& date, const TimeDuration& time);
            DateTime(const detail::time_point_t& time_point);
            virtual ~DateTime();

            DateTime& operator = (const DateTime& other);

            bool operator == (const DateTime& other) const;
            bool operator != (const DateTime& other) const;
            bool operator < (const DateTime& other) const;
            bool operator <= (const DateTime& other) const;
            bool operator > (const DateTime& other) const;
            bool operator >= (const DateTime& other) const;

            DateTime& operator += (const TimeDuration& duration);
            DateTime& operator -= (const TimeDuration& duration);

            DateTime operator + (const TimeDuration& duration) const;
            DateTime operator - (const TimeDuration& duration) const;
            TimeDuration operator - (const DateTime& other) const;
          
            Date date() const;
            TimeDuration time_of_day() const;

            std::time_t as_time_t() const;
            std::tm as_tm() const;

            std::string as_string() const;
            std::string as_iso_string() const;
            std::string as_iso_extended_string() const;

            static DateTime from_tm(const std::tm& tm);
            static DateTime from_string(const std::string& str);
            static DateTime from_iso_string(const std::string& str);
            static DateTime from_iso_extended_string(const std::string& str);

            static DateTime from_stream(std::istream& is, bool assume_eoi = true);

            detail::time_point_t get_impl() const { return m_time_point; }

            static const DateTime epoch;
        private:
            detail::time_point_t m_time_point;
        };

        inline std::tm to_tm(const DateTime& dt) { return dt.as_tm(); }
        std::string to_simple_string(const DateTime& dt);
        std::string to_iso_string(const DateTime& dt);
        std::string to_iso_extended_string(const DateTime& dt);

        DateTime time_from_string(const std::string& str);
        DateTime time_from_iso_string(const std::string& str);
        DateTime time_from_iso_extended_string(const std::string& str);

        DateTime from_time_t(std::time_t t);

        std::ostream& operator<<(std::ostream& os, const DateTime& dt);

        namespace MicrosecClock
        {
            DateTime universal_time();
            DateTime local_time();
        };

        namespace SecondClock
        {
            DateTime universal_time();
            DateTime local_time();
        };

    }
}
