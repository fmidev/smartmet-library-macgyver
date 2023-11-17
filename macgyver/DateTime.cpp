#include "DateTime.h"

using namespace Fmi::detail;
namespace pt = boost::posix_time;
namespace g = boost::gregorian;

namespace
{
    std::string maybe_discard_seconds_part(std::string&& src)
    {
        const std::size_t pos = src.find_last_not_of("0");
        if (pos == std::string::npos) {
            return src;
        } else if (src[pos] == '.') {
            return std::string(src.begin(), src.begin() + pos);
        } else {
            return src;
        }
    }
}

const time_point_t epoch_time_point(
    Fmi::DateTimeNS::local_days(year_t(1970) / month_t(1) / day_t(1)));

const pt::ptime Fmi::detail::epoch_ptime(g::date(1970, 1, 1));

Fmi::detail::time_point_t
Fmi::detail::ptime_to_time_point(const pt::ptime& ptm)
{
    return Fmi::detail::time_point_t(
        Fmi::DateTimeNS::local_days(
            year_t(ptm.date().year()) / month_t(ptm.date().month()) / day_t(ptm.date().day())) + 
        duration_t(ptm.time_of_day().total_microseconds()));
}

boost::posix_time::ptime Fmi::detail::time_point_to_ptime(const time_point_t& time_point)
{
    return epoch_ptime + pt::microseconds(time_point.time_since_epoch().count());
}

Fmi::detail::TimeType Fmi::detail::get_time_type(const DateTime& time)
{
    if (time.is_special())
    {
        if (time.is_pos_infinity())
            return TimeType::POS_INFINITY;
        else if (time.is_neg_infinity())
            return TimeType::NEG_INFINITY;
        return TimeType::NOT_A_DATE_TIME;
    }
    else
    {
        return TimeType::NORMAL;
    }
}

Fmi::detail::TimeType Fmi::detail::get_time_type(boost::posix_time::special_values sv)
{
    switch (sv)
    {
        case pt::not_special:
            return TimeType::NORMAL;
        case pt::neg_infin:
        case pt::min_date_time:
            return TimeType::NEG_INFINITY;
        case pt::pos_infin:
        case pt::max_date_time:
            return TimeType::POS_INFINITY;
        default:
            return TimeType::NOT_A_DATE_TIME;
    }
}

std::string Fmi::detail::to_iso_string(const Fmi::detail::time_point_t& time)
{
    return maybe_discard_seconds_part(
        Fmi::DateTimeNS::format("%Y%m%dT%H%M%S", time));
}

std::string Fmi::detail::to_iso_extended_string(const Fmi::detail::time_point_t& time)
{
    return maybe_discard_seconds_part(
        Fmi::DateTimeNS::format("%Y-%m-%dT%H:%M:%S", time));
}

