#pragma once

// STL modules
#include <cmath>

// Custom modules
#include "common/config.hpp"
#include "common/i2c.hpp"
#include "common/utility.hpp"

namespace kc {

namespace Sensors
{
    // Count of digits in decimal part of values
    constexpr int Precision = 2;

    enum class Location
    {
        External,
        Internal,
    };

    struct Aht20Measurement
    {
        double temperature = 0.0;   // Temperature in celsius degrees
        double humidity = 0.0;      // Relative humidity in percents

        /// @brief Substract other AHT20 measurement from this AHT20 measurement
        /// @param other The other AHT20 measurement to substract
        /// @return Result of substraction
        Aht20Measurement& operator-=(const Aht20Measurement& other);

        /// @brief Add other AHT20 measurement to this AHT20 measurement
        /// @param other The other AHT20 measurement to add
        /// @return Reference to this AHT20 measurement
        Aht20Measurement& operator+=(const Aht20Measurement& other);

        /// @brief Divide this AHT20 measurement by number
        /// @param number The number to divide by
        /// @return Reference to this AHT20 measurement
        Aht20Measurement& operator/=(double number);
    };

    struct Bmp280Measurement
    {
        double temperature = 0.0;   // Temperature in celsius degrees
        double pressure = 0.0;      // Air pressure in hectopascales

        /// @brief Substract other BMP280 measurement from this BMP280 measurement
        /// @param other The other BMP280 measurement to substract
        /// @return Result of substraction
        Bmp280Measurement& operator-=(const Bmp280Measurement& other);

        /// @brief Add other BMP280 measurement to this BMP280 measurement
        /// @param other The other BMP280 measurement to add
        /// @return Reference to this BMP280 measurement
        Bmp280Measurement& operator+=(const Bmp280Measurement& other);

        /// @brief Divide this BMP280 measurement by number
        /// @param number The number to divide by
        /// @return Reference to this BMP280 measurement
        Bmp280Measurement& operator/=(double number);
    };

    struct Measurement
    {
        Aht20Measurement aht20;
        Bmp280Measurement bmp280;

        /// @brief Round measurement values
        void round();

        /// @brief Substract other measurement from this measurement
        /// @param other The other measurement to substract
        /// @return Result of substraction
        Measurement& operator-=(const Measurement& other);

        /// @brief Add other measurement to this measurement
        /// @param other The other measurement to add
        /// @return Reference to this measurement
        Measurement& operator+=(const Measurement& other);

        /// @brief Divide this measurement by number
        /// @param number The number to divide by
        /// @return Reference to this measurement
        Measurement& operator/=(double number);
    };

    /// @brief Peform sensors measurement
    /// @param location The location to measure
    /// @param iterations Count of measurement iterations
    /// @return Sensors measurement
    Measurement Measure(Location location, int iterations = 1);
}

} // namespace kc
