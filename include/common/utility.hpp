#pragma once

// STL modules
#include <optional>
#include <chrono>
#include <thread>
#include <cmath>
#include <mutex>
#include <condition_variable>

// Library spdlog
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace kc {

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
    /// @param seconds Amount of seconds to sleep for
    /// @return Whether or not the sleep was interrupted
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
}

} // namespace kc
