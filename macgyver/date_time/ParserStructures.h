#pragma once

#include <cstdlib>
#include <optional>
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

    struct seconds_members_t
    {
        unsigned seconds;
        std::string frac_sec;

        operator double() const
        {
            return seconds + (frac_sec.empty() ? 0 : std::stod("0." + frac_sec));
        }
    };

    struct duration_members_t
    {
        unsigned hours;
        unsigned minutes;
        std::optional<seconds_members_t> seconds;

        int get_seconds() const
        {
            return seconds ? seconds->seconds : 0;
        }

        int get_mks() const
        {
            return seconds ? std::stod("0." + seconds->frac_sec) * 1e6 : 0;
        }
    };

    struct time_zone_offset_members_t
    {
        char sign;
        unsigned hours;
        unsigned minutes;
    };

    struct date_time_members_t
    {
        date_members_t date;
        std::optional<duration_members_t> time;
        std::optional<time_zone_offset_members_t> tz_offset;

        unsigned hours() const
        {
            return time ? time->hours : 0;
        }

        unsigned minutes() const
        {
            return time ? time->minutes : 0;
        }

        unsigned seconds() const
        {
            return time ? time->get_seconds() : 0;
        }

        unsigned mks() const
        {
            return time ? time->get_mks() : 0;
        }

        int offset_hours() const
        {
            return tz_offset ? tz_offset->hours : 0;
        }

        int offset_minutes() const
        {
            return tz_offset ? tz_offset->minutes : 0;
        }
    };

} // namespace parser
} // namespace date_time
} // namespace Fmi
