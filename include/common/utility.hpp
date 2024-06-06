#pragma once

// STL modules
#include <optional>
#include <chrono>
#include <thread>
#include <cmath>

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

    /// @brief Delay execution for some time
    /// @param seconds Amount of seconds to delay for
    void Sleep(double seconds);

    /// @brief Round value to decimal places
    /// @param value The value to round
    /// @param decimalPlacesCount Count of decimal places to round to
    /// @return Rounded value
    double Round(double value, int decimalPlacesCount);
}

} // namespace kc
