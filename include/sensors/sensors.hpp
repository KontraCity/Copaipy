#pragma once

namespace cp {

namespace Sensors {
    // Count of digits in decimal part of values
    constexpr int Precision = 2;

    enum class Location {
        External,
        Internal,
    };

    struct Aht20Measurement {
        double temperature = 0.0;   // Temperature in celsius degrees
        double humidity = 0.0;      // Relative humidity in percents

        Aht20Measurement& operator-=(const Aht20Measurement& other);

        Aht20Measurement& operator+=(const Aht20Measurement& other);

        Aht20Measurement& operator/=(double number);
    };

    struct Bmp280Measurement {
        double temperature = 0.0;   // Temperature in celsius degrees
        double pressure = 0.0;      // Air pressure in hectopascales

        Bmp280Measurement& operator-=(const Bmp280Measurement& other);

        Bmp280Measurement& operator+=(const Bmp280Measurement& other);

        Bmp280Measurement& operator/=(double number);
    };

    struct Measurement {
        Aht20Measurement aht20;
        Bmp280Measurement bmp280;

        void round();

        Measurement& operator-=(const Measurement& other);

        Measurement& operator+=(const Measurement& other);

        Measurement& operator/=(double number);
    };

    Measurement Measure(Location location, int iterations = 1);
}

} // namespace cp
