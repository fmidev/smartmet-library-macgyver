#pragma once

#include <chrono>
#include <sstream>
#include <string>

#define FMI_NEW_DATE_TIME 1

#if __cplusplus >= 202002L && defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L
// Time zone, zoned_time etc is supported in libstdc++ => use it

#define FMI_CALENDAR_USES_STD_CHRONO 1
namespace date = std::chrono;

#else
// Use date library

#ifdef _WIN32
#define USE_OS_TZDB 0
#else
#define USE_OS_TZDB 1
#endif

// Time zone, zoned_time etc is NOT supported in libstdc++ or support is incomplete
//      => use date library
#include "date/date.h"
#include "date/tz.h"

#endif

namespace Fmi
{
    namespace detail
    {

        using period_t = std::micro;
        using duration_t = std::chrono::duration<int64_t, std::micro>;
        using time_point_t = std::chrono::time_point<date::local_t, duration_t>;
        using sys_time_t = std::chrono::time_point<std::chrono::system_clock>;
        using zoned_time_t = date::zoned_time<duration_t>;
        using microsec_t = std::chrono::duration<int64_t, std::micro>;
        using millisec_t = std::chrono::duration<int64_t, std::milli>;
        using seconds_t = std::chrono::duration<int64_t, std::ratio<1, 1> >;
        using minutes_t = std::chrono::duration<int64_t, std::ratio<60, 1> >;
        using hours_t = std::chrono::duration<int64_t, std::ratio<3600, 1> >;
        using days_t = std::chrono::duration<int64_t, std::ratio<86400, 1> >;
        using day_t = date::day;
        using month_t = date::month;
        using year_t = date::year;
        using ymd_t = date::year_month_day;

        using hh_mm_ss = date::hh_mm_ss<duration_t>;

        constexpr int periods_per_sec = period_t::den;
        constexpr int periods_per_mks = std::micro::den * period_t::num / period_t::den;

        extern const time_point_t epoch_time_point;

        static_assert (periods_per_mks * period_t::den / period_t::num == std::micro::den,
          "INTERNAL ERROR");

        constexpr int count_digits(uint64_t den) { return (den == 0 ? 0 : 1 + count_digits(den / 10)); }

    }  // namespace detail

    namespace date_time
    {
        using Weekday = date::weekday;

        constexpr unsigned num_fractional_digits = detail::count_digits(detail::periods_per_sec - 1);

        class Base
        {
        public:
            enum Type
            {
                NORMAL = 0,
                POS_INFINITY = 1,
                NEG_INFINITY = -1,
                NOT_A_DATE_TIME = 2
            };

            Base(Type type = NOT_A_DATE_TIME) : m_type(type)
            {
            }

            virtual ~Base() = default;

            bool is_special() const
            {
                return m_type != NORMAL;
            }

            bool is_pos_infinity() const
            {
                return m_type == POS_INFINITY;
            }

            bool is_neg_infinity() const
            {
                return m_type == NEG_INFINITY;
            }

            bool is_infinity() const
            {
                return m_type == POS_INFINITY || m_type == NEG_INFINITY;
            }

            bool is_not_a_date_time() const
            {
                return m_type == NOT_A_DATE_TIME;
            }

            inline Type type() const { return m_type; }

        protected:
            std::string as_string() const;

            inline void set_type(Type type)
            {
                m_type = type;
            }

            bool operator == (const Base& other) const;
            bool operator != (const Base& other) const;
            bool operator < (const Base& other) const;
            bool operator <= (const Base& other) const;
            bool operator > (const Base& other) const;
            bool operator >= (const Base& other) const;

        private:
            void assert_supported() const;
            void assert_supported(const Base& other) const;

        private:
            Type m_type;
        };
    }
}
