#pragma once

#include "Base.h"
#include "TimeDuration.h"
#include "DateTime.h"
#include "TimeZonePtr.h"

#include <string>

namespace Fmi
{
    namespace date_time
    {
        class TimeZonePtr;

        class LocalDateTime : public Base
        {
        public:
            enum ErrorHandling
            {
                EXCEPTION_ON_ERROR,
                NOT_DATE_TIME_ON_ERROR
            };            

            LocalDateTime();

            explicit LocalDateTime(Type type);

            LocalDateTime(const LocalDateTime& src);

            LocalDateTime(const detail::zoned_time_t& zoned_time);

            LocalDateTime(
                const DateTime& time,
                const TimeZonePtr& tz,
                enum ErrorHandling err_handling = NOT_DATE_TIME_ON_ERROR);

            LocalDateTime(
                const Date& date,
                const TimeDuration& time,
                const TimeZonePtr& tz,
                enum ErrorHandling err_handling = NOT_DATE_TIME_ON_ERROR);

            LocalDateTime(
                const detail::time_point_t& time,
                const date::time_zone* tz,
                enum ErrorHandling err_handling = NOT_DATE_TIME_ON_ERROR);
          
            virtual ~LocalDateTime();

            LocalDateTime& operator = (const LocalDateTime& src);

            /**
             * Get the internal representation of the time point (date::zoned_time)
             */
            const detail::zoned_time_t& get_impl() const;

            TimeZonePtr zone() const;

            TimeDuration time_of_day() const;

            /**
             * Get the UTC time of the local date time
            */
            DateTime utc_time() const;

            /**
             * Get the local time of the local date time
            */
            DateTime local_time() const;

            /**
             * Get the date of the local date time
            */
            Date date() const;

            /**
             * Convert the local date time to the specified time zone
             * 
             *  @param zone Time zone to convert to
             *  @return Local date time in the specified time zone
             *  @exception Fmi::Exception if operation failed
             */
            LocalDateTime to_tz(const TimeZonePtr& zone) const;

            /**
             *  Query whether DST is in effect
            */
            bool dst_on() const;

            /**
             *  Get the time zone offset with possible DST offset included
             */
            TimeDuration offset() const;

#if !USE_OS_TZDB
            /**
             *   Get the DST offset
             * 
             *   Not supported for OS TZDB database
            */

            TimeDuration dst_offset() const;
#endif

            std::string abbrev() const;

            date::sys_info get_sys_info() const;

            std::pair<detail::sys_time_t, detail::sys_time_t> get_dst_times() const;

            std::string as_simple_string() const;

            std::string as_iso_string() const;

            std::string as_iso_extended_string() const;

            /**
             *  Add specified time duration to the local date time
             * 
             *  @param td Time duration to add
             *  @exception Fmi::Exception if local date time is not a normal date time or operration failed
             */            
            void advance(const TimeDuration& td);

            inline LocalDateTime& operator += (const TimeDuration& td) { advance(td); return *this; }
            inline LocalDateTime& operator -= (const TimeDuration& td) { advance(-td); return *this; }

            bool operator < (const LocalDateTime& other) const;
            bool operator > (const LocalDateTime& other) const;
            bool operator <= (const LocalDateTime& other) const;
            bool operator >= (const LocalDateTime& other) const;
            bool operator == (const LocalDateTime& other) const;
            bool operator != (const LocalDateTime& other) const;

        private:
            int compare_with(const LocalDateTime& other) const;

            void check_no_special(const char* _filename, int _line, const char* _function) const;

            detail::zoned_time_t ldt;
        };

        LocalDateTime operator + (const LocalDateTime& time, const TimeDuration& td);
        LocalDateTime operator - (const LocalDateTime& time, const TimeDuration& td);
        TimeDuration operator - (const LocalDateTime& to, const LocalDateTime& from);

        std::string to_simple_string(const LocalDateTime& time);
        std::string to_iso_string(const LocalDateTime& time);
        std::string to_iso_extended_string(const LocalDateTime& time);

        inline TimeDuration seconds(int s) { return TimeDuration(detail::duration_t(std::chrono::seconds(s))); }
        inline TimeDuration minutes(int m) { return TimeDuration(detail::duration_t(std::chrono::minutes(m))); }
        inline TimeDuration hours(int h) { return TimeDuration(detail::duration_t(std::chrono::hours(h))); }

        std::ostream& operator << (std::ostream& os, const LocalDateTime& time);

        /**
         *  Make a local date time from a date and time of day (preffers summer time in case of ambiguity)
         *
         *  Returns NOT_A_DATE_TIME if fails to create a valid local date time
         *
         *  @param date Date
         *  @param time Time of day
         *  @param tz Time zone
         *  @return Local date time
         */
        LocalDateTime make_time(const Fmi::date_time::Date& date, const Fmi::date_time::TimeDuration& time,
                                const Fmi::date_time::TimeZonePtr& tz);

    }
}
