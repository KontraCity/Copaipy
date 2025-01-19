#pragma once

#include <memory>
#include <string>

namespace cp {

namespace ConfigConst {
    constexpr const char* ConfigFile = "config.json";

    namespace Objects {
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

    namespace Defaults {
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

class Config {
public:
    static const std::unique_ptr<Config> Instance;

public:
    static void GenerateSampleFile();

private:
    std::string m_error;
    uint16_t m_httpPort;
    int m_timeReserve;
    std::string m_externalPort;
    std::string m_internalPort;
    double m_latitude;
    double m_longitude;
    double m_sunriseAngle;
    double m_sunsetAngle;

private:
    Config();

public:
    inline const std::string& error() const {
        return m_error;
    }

    inline uint16_t httpPort() const {
        return m_httpPort;
    }

    inline int timeReserve() const {
        return m_timeReserve;
    }

    inline const std::string& externalPort() const {
        return m_externalPort;
    }

    inline const std::string& internalPort() const {
        return m_internalPort;
    }

    inline double latitude() const {
        return m_latitude;
    }

    inline double longitude() const {
        return m_longitude;
    }

    inline double sunriseAngle() const {
        return m_sunriseAngle;
    }

    inline double sunsetAngle() const {
        return m_sunsetAngle;
    }
};

} // namespace cp
