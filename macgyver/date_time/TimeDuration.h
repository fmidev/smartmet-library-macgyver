#pragma once

#include <ctime>
#include <string>
#include "Base.h"

namespace Fmi
{
    namespace date_time
    {
        class TimeDuration : public Base
        {
        public:
            TimeDuration() = default;
            TimeDuration(Type type) : Base(type) {}
            TimeDuration(const detail::duration_t& duration);
            TimeDuration(const TimeDuration& other) = default;
            TimeDuration(int hours, int minutes, int seconds, int microseconds = 0);
            virtual ~TimeDuration() = default;

            TimeDuration& operator=(const TimeDuration& other) = default;

            int64_t hours() const;
            int64_t minutes() const;
            int64_t seconds() const;
            int64_t fractional_seconds() const;

            int64_t total_seconds() const;
            int64_t total_milliseconds() const;
            int64_t total_microseconds() const;

            constexpr int64_t ticks_per_second() const { return detail::period_t::den; }

            virtual std::string as_string() const;
            std::string as_iso_string() const;
            std::string as_iso_extended_string() const;

            bool operator==(const TimeDuration& other) const;
            bool operator!=(const TimeDuration& other) const;
            bool operator<(const TimeDuration& other) const;
            bool operator<=(const TimeDuration& other) const;
            bool operator>(const TimeDuration& other) const;
            bool operator>=(const TimeDuration& other) const;

            TimeDuration& operator + () { return *this; }
            TimeDuration operator - () const { return TimeDuration(-m_duration);}

            TimeDuration operator+(const TimeDuration& other) const;
            TimeDuration operator-(const TimeDuration& other) const;
            TimeDuration operator*(int64_t factor) const;
            TimeDuration operator/(int64_t factor) const;

            TimeDuration& operator+=(const TimeDuration& other);
            TimeDuration& operator-=(const TimeDuration& other);
            TimeDuration& operator*=(int64_t factor);
            TimeDuration& operator/=(int64_t factor);

            detail::duration_t get_impl() const { return m_duration; }

            /**
             * @brief  Parse time duration from string.
             * 
             * @param str  String to parse.
             * @param assume_eoi  If true, allows only whitespace after the duration.
             * @return TimeDuration  Parsed time duration.
            */
            static TimeDuration from_stream(std::istream& src, bool assume_eoi = true);

        private:
            void assert_special() const;

        private:
            detail::duration_t m_duration;
        };

        TimeDuration days(int days);
        TimeDuration hours(int hours);
        TimeDuration minutes(int minutes);
        TimeDuration seconds(int seconds);
        TimeDuration milliseconds(int milliseconds);
        TimeDuration microseconds(int microseconds);

        TimeDuration duration_from_string(const std::string& str);

        struct std::tm to_tm(const TimeDuration& td);

        std::string to_string(const TimeDuration& td);
        std::string to_iso_string(const TimeDuration& td);

        std::ostream& operator<<(std::ostream& os, const TimeDuration& td);
    }
}
