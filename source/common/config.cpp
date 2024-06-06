#include "common/config.hpp"
using namespace kc::ConfigConst;

namespace kc {

void Config::GenerateSampleFile()
{
    std::ofstream configFile(ConfigFile);
    if (!configFile)
        throw std::runtime_error("kc::Config::GenerateSampleFile(): Couldn't create sample configuration file");

    json commonObject;
    commonObject[Objects::HttpPort] = Defaults::HttpPort;
    commonObject[Objects::TimeReserve] = Defaults::TimeReserve;

    json i2cPortsObject;
    i2cPortsObject[Objects::ExternalPort] = Defaults::ExternalPort;
    i2cPortsObject[Objects::InternalPort] = Defaults::InternalPort;

    json locationObject;
    locationObject[Objects::Latitude] = Defaults::Latitude;
    locationObject[Objects::Longitude] = Defaults::Longitude;

    json sunObject;
    sunObject[Objects::SunriseAngle] = Defaults::SunriseAngle;
    sunObject[Objects::SunsetAngle] = Defaults::SunsetAngle;

    json configJson;
    configJson[Objects::Common] = commonObject;
    configJson[Objects::I2CPorts] = i2cPortsObject;
    configJson[Objects::Location] = locationObject;
    configJson[Objects::Sun] = sunObject;
    configFile << configJson.dump(4) << '\n';
}

Config::Config()
{
    std::ifstream configFile(ConfigFile);
    if (!configFile)
        throw Error(fmt::format("Couldn't open configuration file \"{}\"", ConfigFile).c_str());

    try
    {
        const json configJson = json::parse(configFile);

        const json& commonObject = configJson.at(Objects::Common);
        m_httpPort = commonObject.at(Objects::HttpPort);
        m_timeReserve = commonObject.at(Objects::TimeReserve);

        const json& i2cPortsObject = configJson.at(Objects::I2CPorts);
        m_externalPort = i2cPortsObject.at(Objects::ExternalPort);
        m_internalPort = i2cPortsObject.at(Objects::InternalPort);

        const json& locationObject = configJson.at(Objects::Location);
        m_latitude = locationObject.at(Objects::Latitude);
        m_longitude = locationObject.at(Objects::Longitude);

        const json& sunObject = configJson.at(Objects::Sun);
        m_sunriseAngle = sunObject.at(Objects::SunriseAngle);
        m_sunsetAngle = sunObject.at(Objects::SunsetAngle);
    }
    catch (const json::exception&)
    {
        throw Error(fmt::format("Couldn't parse configuration file \"{}\" JSON", ConfigFile).c_str());
    }

    if (m_timeReserve < 0)
        throw Error(fmt::format("Time reserve value can't be negative (current: {})", m_timeReserve).c_str());
    if (m_latitude < -90.0 || m_latitude > 90.0)
        throw Error(fmt::format("Latitude value is not in range (current: {}, range: [-90; 90])", m_latitude).c_str());
    if (m_longitude < -180.0 || m_longitude > 180.0)
        throw Error(fmt::format("Longitude value is not in range (current: {}, range: [-180; 180])", m_longitude).c_str());
    if (m_sunriseAngle < 80.0 || m_sunriseAngle > 94.7)
        throw Error(fmt::format("Target sunrise angle value is not in range (current: {}, range: [80; 94.7])", m_sunriseAngle).c_str());
    if (m_sunsetAngle < 80.0 || m_sunsetAngle > 94.7)
        throw Error(fmt::format("Target sunset angle value is not in range (current: {}, range: [80; 94.7])", m_sunsetAngle).c_str());
}

} // namespace kc
