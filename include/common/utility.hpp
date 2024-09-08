#pragma once

// STL modules
#include <optional>
#include <chrono>
#include <thread>
#include <cmath>
#include <algorithm>
#include <mutex>
#include <condition_variable>

// Library spdlog
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

// Library Boost.Date_Time
#include <boost/date_time.hpp>

namespace kc {

/* Namespace aliases and imports */
namespace dt = boost::gregorian;
namespace pt = boost::posix_time;

namespace Utility
{
    /// @brief Create a logger
    /// @param name Logger name
    /// @param forceColor Whether to force sinks colors or not
    /// @return Created logger
    spdlog::logger CreateLogger(const std::string& name, std::optional<bool> forceColor = {});

    /// @brief Perform execution delay
    /// @param seconds Amount of seconds to delay for
    void Sleep(double seconds);
    
    /// @brief Perform interruptable execution delay
    /// @param lock Mutex lock to use
    /// @param cv Condition variable to use
    /// @param seconds Amount of seconds to delay for
    /// @return True if the delay was interrupted
    bool InterSleep(std::unique_lock<std::mutex>& lock, std::condition_variable& cv, double seconds);

    /// @brief Round value to decimal places
    /// @param value The value to round
    /// @param decimalPlacesCount Count of decimal places to round to
    /// @return Rounded value
    double Round(double value, int decimalPlacesCount);

    /// @brief Limit value to an inclusive range
    /// @param value The value to limit
    /// @param min Minimum range bound
    /// @param max Maximum range bound
    /// @return Limited value
    double Limit(double value, double min, double max);

    /// @brief Get local timezone offset in hours
    /// @return Local timezone offset in hours
    int GetTimezoneOffset();

    /// @brief Convert date to string
    /// @param date The date to convert
    /// @return Converted string
    std::string ToString(dt::date date);

    /// @brief Convert duration to string
    /// @param duration The duration to convert
    /// @param force Whether to convert all fields or not
    /// @return Converted string
    std::string ToString(pt::time_duration duration, bool force = false);

    /// @brief Convert timestamp to string
    /// @param timestamp The timestamp to convert
    /// @return Converted string
    std::string ToString(pt::ptime timestamp);

    /// @brief Convert timestamp to filename
    /// @param timestamp The timestamp to convert
    /// @return Converted filename
    std::string ToFilename(pt::ptime timestamp);

    /// @brief Truncate string
    /// @param string The string to truncate
    /// @param maxLength Maximum string length
    /// @param end Whether to truncate at end or not (at start if false)
    /// @return Truncated string
    std::string Truncate(const std::string& string, size_t maxLength, bool end = true);
}

} // namespace kc
