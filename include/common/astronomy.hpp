#pragma once

#include <boost/date_time.hpp>
namespace dt = boost::gregorian;
namespace pt = boost::posix_time;

namespace cp {

namespace Astronomy {
    pt::ptime CalculateSunrise(dt::date date);

    pt::ptime CalculateSunset(dt::date date);
}

} // namespace cp
