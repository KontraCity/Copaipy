#pragma once

// STL modules
#include <memory>
#include <string>
#include <fstream>
#include <stdexcept>

// Library nlohmann::json
#include <nlohmann/json.hpp>

// Library {fmt}
#include <fmt/format.h>

namespace kc {

/* Namespace aliases and imports */
using nlohmann::json;

namespace ConfigConst
{
    // Copaipy configuration JSON file name
    constexpr const char* ConfigFile = "config.json";

    namespace Objects
    {
        constexpr const char* Common = "common";
        constexpr const char* HttpPort = "http_port";
        constexpr const char* TimeReserve = "time_reserve";

        constexpr const char* I2CPorts = "i2c_ports";
        constexpr const char* ExternalPort = "external";
        constexpr const char* InternalPort = "internal";

        constexpr const char* Location = "location";
        constexpr const char* Latitude = "latitude";
        constexpr const char* Longitude = "longitude";

        constexpr const char* Sun = "sun";
        constexpr const char* SunriseAngle = "sunrise_angle";
        constexpr const char* SunsetAngle = "sunset_angle";
    }

    namespace Defaults
    {
        constexpr uint16_t HttpPort = 80;
        constexpr int TimeReserve = 5000;

        constexpr const char* ExternalPort = "i2c-1";
        constexpr const char* InternalPort = "i2c-3";

        constexpr double Latitude = 0.0;
        constexpr double Longitude = 0.0;

        constexpr double SunriseAngle = 90.833;
        constexpr double SunsetAngle = 90.833;
    }
}

class Config
{
public:
    // Shared config instance pointer
    using Pointer = std::shared_ptr<Config>;

    // Configuration file read/parse error
    class Error : public std::logic_error
    {
    public:
        using logic_error::logic_error;
    };

public:
    /// @brief Generate sample configuration file for user to fill out
    /// @throw std::runtime_error if file couldn't bee created
    static void GenerateSampleFile();

private:
    uint16_t m_httpPort;
    int m_timeReserve;
    std::string m_externalPort;
    std::string m_internalPort;
    double m_latitude;
    double m_longitude;
    double m_sunriseAngle;
    double m_sunsetAngle;

public:
    /// @brief Read and parse configuration file
    /// @throw kc::Config::Error if reading/parsing error occurs
    Config();

    /// @brief Get HTTP server port
    /// @return HTTP server port
    inline uint16_t httpPort() const
    {
        return m_httpPort;
    }

    /// @brief Get capture time reserve
    /// @return Capture time reserve
    inline int timeReserve() const
    {
        return m_timeReserve;
    }

    /// @brief Get external I2C port
    /// @return External I2C port
    inline const std::string& externalPort() const
    {
        return m_externalPort;
    }

    /// @brief Get internal I2C port
    /// @return Internal I2C port
    inline const std::string& internalPort() const
    {
        return m_internalPort;
    }

    /// @brief Get system location latitude
    /// @return System location latitude
    inline double latitude() const
    {
        return m_latitude;
    }

    /// @brief Get system location longitude
    /// @return System location longitude
    inline double longitude() const
    {
        return m_longitude;
    }

    /// @brief Get target sunrise angle
    /// @return Target sunrise angle
    inline double sunriseAngle() const
    {
        return m_sunriseAngle;
    }

    /// @brief Get target sunset angle
    /// @return Target sunset angle
    inline double sunsetAngle() const
    {
        return m_sunsetAngle;
    }
};

} // namespace kc
