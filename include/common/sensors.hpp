#pragma once

// STL modules
#include <cmath>
#include <mutex>

// Custom modules
#include "common/config.hpp"
#include "common/i2c.hpp"
#include "common/utility.hpp"

namespace kc {

namespace Sensors
{
    enum class Location
    {
        External,
        Internal,
    };

    struct Aht20Measurement
    {
        double temperature = 0.0;   // Temperature in celsius degrees
        double humidity = 0.0;      // Relative humidity in percents
    };

    struct Bmp280Measurement
    {
        double temperature = 0.0;   // Temperature in celsius degrees
        double pressure = 0.0;      // Air pressure in hectopascales
    };

    struct Measurement
    {
        Aht20Measurement aht20;
        Bmp280Measurement bmp280;
    };

    /// @brief Peform sensors measurement
    /// @param config System config
    /// @param location Location to measure
    /// @return Sensors measurement
    Measurement Measure(Config::Pointer config, Location location);
}

} // namespace kc
