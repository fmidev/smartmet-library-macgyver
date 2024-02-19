#pragma once

//#include "DateTime.h"
//#include "LocalDateTime.h"
#include <type_traits>

namespace Fmi
{
    namespace date_time
    {
        class DateTime;

        class LocalDateTime;

        /**
         * A time period defined by a start and end time.
         *
         * The time period is null if the start and end times are not valid or
         * the end time is before the start time.
         *
         * Implementation is based on API of boost::posix_time::time_period
         * and is generated mostly by GitHub copilot with some corrections needed.
         *
         * @fixme do we need also conversions from and to string?
         * @fixme implement some more tests for this class
        */
        template
        <
            typename DateTimeType = DateTime,
            typename std::enable_if<std::is_same<DateTimeType, DateTime>::value
                || std::is_same<DateTimeType, LocalDateTime>::value, int>::type = 0
        >
        class TimePeriod
        {
        public:
            TimePeriod() = default;

            TimePeriod(const DateTimeType& start, const DateTimeType& end)
                : m_start(start),
                  m_end(end)
            {
                if (start.is_special() || end.is_special() || m_end < m_start)
                {
                    m_start = DateTimeType();
                    m_end = DateTimeType();
                }
            }

            TimePeriod(const TimePeriod& other)
                : m_start(other.m_start),
                  m_end(other.m_end)
            {
            }

            virtual ~TimePeriod() = default;

            TimePeriod& operator=(const TimePeriod& other)
            {
                m_start = other.m_start;
                m_end = other.m_end;
                return *this;
            }

            DateTimeType begin() const
            {
                return m_start;
            }

            DateTimeType end() const
            {
                return m_end;
            }

            /**
             * Shift the time period by the given duration.
            */
            void shift(const TimeDuration& duration)
            {
                if (m_start.is_special() || m_end.is_special())
                    return;

                m_start += duration;
                m_end += duration;
            }

            /**
             * Expand the time period by the given duration.
            */
            void expand(const TimeDuration& duration)
            {
                if (m_start.is_special() || m_end.is_special())
                    return;

                m_start -= duration;
                m_end += duration;
            }

            /**
             * Get the length of the time period.
            */
            TimeDuration length() const
            {
                if (m_start.is_special() || m_end.is_special())
                    return TimeDuration();
                return m_end - m_start;
            }

            /**
             * Check if the time period is null.
            */
            bool is_null() const
            {
                return m_start.is_special() || m_end.is_special();
            }

            /**
             * Check if the time period contains the given time.
            */
            bool contains(const DateTimeType& time) const
            {
                if (m_start.is_special() || m_end.is_special() || time.is_special())
                    return false;
                return m_start <= time && time <= m_end;
            }

            /**
             * Check if the time period contains the given time period.
             */
            bool contains(const TimePeriod& period) const
            {
                if (m_start.is_special() || m_end.is_special() || period.m_start.is_special() || period.m_end.is_special())
                    return false;
                return m_start <= period.m_start && period.m_end <= m_end;
            }

            /**
             * Check if the time period intersects the given time period.
             */
            bool intersects(const TimePeriod& period) const
            {
                if (m_start.is_special() || m_end.is_special() || period.m_start.is_special() || period.m_end.is_special())
                    return false;
                return m_start < period.m_end && period.m_start < m_end;
            }

            /**
             * Get the intersection of the time periods.
            */
            TimePeriod intersection(const TimePeriod& period) const
            {
                if (m_start.is_special() || m_end.is_special() || period.m_start.is_special() || period.m_end.is_special())
                    return TimePeriod();
                DateTimeType start = m_start > period.m_start ? m_start : period.m_start;
                DateTimeType end = m_end < period.m_end ? m_end : period.m_end;
                return TimePeriod(start, end);
            }

            /**
             * Merge the time periods.
             * 
             * Note: returns a null time period if the time periods do not intersect.
            */
            TimePeriod merge(const TimePeriod& period) const
            {
                if (m_start.is_special() || m_end.is_special() || period.m_start.is_special() || period.m_end.is_special())
                    return TimePeriod();
                if (intersects(period))
                {
                    DateTimeType start = m_start < period.m_start ? m_start : period.m_start;
                    DateTimeType end = m_end > period.m_end ? m_end : period.m_end;
                    return TimePeriod(start, end);
                }
                else
                {
                    return TimePeriod();
                }
            }

            /**
             * Get the span of the time periods (the smallest time
             * period that contains both).
            */
            TimePeriod span(const TimePeriod& period) const
            {
                if (m_start.is_special() || m_end.is_special() || period.m_start.is_special() || period.m_end.is_special())
                    return TimePeriod();
                DateTimeType start = m_start < period.m_start ? m_start : period.m_start;
                DateTimeType end = m_end > period.m_end ? m_end : period.m_end;
                return TimePeriod(start, end);
            }

            /**
             * Check if the time periods are equal.
            */
            bool operator==(const TimePeriod& other) const
            {
                return m_start == other.m_start && m_end == other.m_end;
            }

            /**
             * Check if the time periods are different.
            */
            bool operator!=(const TimePeriod& other) const
            {
                return m_start != other.m_start || m_end != other.m_end;
            }

            /**
             * Compare the time periods (without overlap)
             **/
            bool operator<(const TimePeriod& other) const
            {
                return m_end < other.m_start;
            }

            /**
             * Compare the time periods (including overlap)
            */
            bool operator<=(const TimePeriod& other) const
            {
                return m_end <= other.m_start;
            }

            /**
             * Compare the time periods (without overlap)
            */
            bool operator>(const TimePeriod& other) const
            {
                return m_start > other.m_end;
            }

            /**
             * Compare the time periods (including overlap)
            */
            bool operator>=(const TimePeriod& other) const
            {
                return m_start >= other.m_end;
            }

        private:
            DateTimeType m_start;
            DateTimeType m_end;
        };
    }
}
