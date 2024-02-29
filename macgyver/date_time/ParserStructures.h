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
    };

} // namespace parser
} // namespace date_time
} // namespace Fmi
