#pragma once

//#define USE_BOOST_DATE_TIME

#ifdef USE_BOOST_DATE_TIME
#include <boost/date_time/posix_time/posix_time.hpp>
namespace Fmi
{
    using Date = boost::gregorian::date;
    using TimeDuration = boost::posix_time::time_duration;
    using DateTime = boost::posix_time::ptime;

    using Seconds = boost::posix_time::seconds;
    using Minutes = boost::posix_time::minutes;
    using Hours = boost::posix_time::hours;

    using MicrosecClock = boost::posix_time::microsec_clock;
    using SecondClock = boost::posix_time::second_clock;

    // FIXME: to be handled later
    //     boost::posix_time::ptime_from_tm
    //     boost::posix_time::from_time_t
    //     boost::date_time::neg_infin
    //     boost::date_time::pos_infin
}

#else

#include "date_time/DateTime.h"

namespace Fmi
{
    using Date = date_time::Date;
    using TimeDuration = date_time::TimeDuration;
    using DateTime = date_time::DateTime;

    using date_time::Seconds;
    using date_time::Minutes;
    using date_time::Hours;

    namespace MicrosecClock = date_time::MicrosecClock;
    namespace SecondClock = date_time::SecondClock;
}

#endif
