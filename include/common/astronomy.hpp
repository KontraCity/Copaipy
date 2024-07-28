#pragma once

// STL modules
#define _USE_MATH_DEFINES
#include <cmath>

// Library Boost.Date_Time
#include <boost/date_time.hpp>

// Custom modules
#include "common/config.hpp"
#include "common/utility.hpp"

namespace kc {

/* Namespace aliases and imports */
namespace dt = boost::gregorian;
namespace pt = boost::posix_time;

namespace Astronomy
{
    /// @brief Calculate sunrise timestamp for a given date
    /// @param config Initialized config
    /// @param date Date to calculate sunrise timestamp for
    /// @return Sunrise timestamp
    pt::ptime CalculateSunrise(Config::Pointer config, dt::date date);

    /// @brief Calculate sunset timestamp for a given date
    /// @param config Initialized config
    /// @param date Date to calculate sunset timestamp for
    /// @return Sunset timestamp
    pt::ptime CalculateSunset(Config::Pointer config, dt::date date);
}

} // namespace kc
