#include "common/astronomy.hpp"

#define _USE_MATH_DEFINES
#include <cmath>

#include "common/config.hpp"
#include "common/utility.hpp"

namespace cp {

static inline double DegreesToRadians(double degrees) {
    return (degrees * M_PI / 180);
}

static inline double RadiansToDegrees(double radians) {
    return (radians * 180 / M_PI);
}

static double GregorianToJulian(dt::date date) {
    int year = date.year(), month = date.month(), day = date.day();
    if (month <= 2) {
        year -= 1;
        month += 12;
    }

    int result = 2 - (year / 100) + (year / 400);
    result += static_cast<int>(365.25 * (year + 4716));
    result += static_cast<int>(30.6001 * (month + 1));
    return result + day - 1524.5;
}

static inline double CalculateJulianCenturies(double julianDate) {
    return (julianDate - 2451545.0) / 36525.0;
}

static double CalculateObliquityCorrection(double julianCenturies) {
    double seconds = 21.448 - julianCenturies * (46.8150 + julianCenturies * (0.00059 - julianCenturies * 0.001813));
    double minutes = 26.0 + seconds / 60.0;
    double degrees = 23.0 + minutes / 60.0;
    return degrees + 0.00256 * std::cos(DegreesToRadians(125.04 - 1934.136 * julianCenturies));
}

static inline double CalculateOrbitEccentricity(double julianCenturies) {
    return 0.016708634 - julianCenturies * (0.000042037 + 0.0000001267 * julianCenturies);
}

static inline double CalculateSolarAnomaly(double julianCenturies) {
    return 357.52911 + julianCenturies * (35999.05029 - 0.0001537 * julianCenturies);
}

static double CalculateSunEquationOfCenter(double julianCenturies) {
    double sunAnomalyRadians = DegreesToRadians(CalculateSolarAnomaly(julianCenturies));
    double result = std::sin(sunAnomalyRadians) * (1.914602 - julianCenturies * (0.004817 + 0.000014 * julianCenturies));
    result += std::sin(2.0 * sunAnomalyRadians) * (0.019993 - 0.000101 * julianCenturies);
    result += std::sin(3.0 * sunAnomalyRadians) * 0.000289;
    return result;
}

static inline double CalculateSunMeanLongitude(double julianCenturies) {
    return std::fmod(280.46646 + julianCenturies * (36000.76983 + 0.0003032 * julianCenturies), 360.0);
}

static inline double CalculateSunTrueLongitude(double julianCenturies) {
    return CalculateSunMeanLongitude(julianCenturies) + CalculateSunEquationOfCenter(julianCenturies);
}

static inline double CalculateSunApparentLongitude(double julianCenturies) {
    return CalculateSunTrueLongitude(julianCenturies) - 0.00569 - 0.00478 * std::sin(DegreesToRadians(125.04 - 1934.136 * julianCenturies));
}

static double CalculateEquationOfTime(double julianCenturies) {
    double obliquityCorrection = CalculateObliquityCorrection(julianCenturies);
    double sunLongitude = CalculateSunMeanLongitude(julianCenturies);
    double orbitEccentricity = CalculateOrbitEccentricity(julianCenturies);
    double solarAnomaly = CalculateSolarAnomaly(julianCenturies);
    double correction = std::pow(std::tan(DegreesToRadians(obliquityCorrection) / 2.0), 2);

    double result = correction * std::sin(2.0 * DegreesToRadians(sunLongitude));
    result -= 2.0 * orbitEccentricity * std::sin(DegreesToRadians(solarAnomaly));
    result += 4.0 * orbitEccentricity * correction * std::sin(DegreesToRadians(solarAnomaly)) * std::cos(2.0 * DegreesToRadians(sunLongitude));
    result -= 0.5 * correction * correction * std::sin(4.0 * DegreesToRadians(sunLongitude));
    result -= 1.25 * orbitEccentricity * orbitEccentricity * std::sin(2.0 * DegreesToRadians(solarAnomaly));
    return RadiansToDegrees(result) * 4.0;
}

static double CalculateSolarDeclination(double julianCenturies) {
    double obliquityCorrection = CalculateObliquityCorrection(julianCenturies);
    double sunApparentLongitude = CalculateSunApparentLongitude(julianCenturies);
    return RadiansToDegrees(std::asin(std::sin(DegreesToRadians(obliquityCorrection)) * std::sin(DegreesToRadians(sunApparentLongitude))));
}

static double CalculateSunEventHourAngle(double latitude, double solarDeclination, double angle, bool sunrise) {
    double latitudeRadians = DegreesToRadians(latitude);
    double solarDeclinationRadians = DegreesToRadians(solarDeclination);

    double result = std::cos(DegreesToRadians(angle)) / (std::cos(latitudeRadians) * std::cos(solarDeclinationRadians));
    result -= std::tan(latitudeRadians) * std::tan(solarDeclinationRadians);
    result = std::acos(result);
    return (sunrise ? result : -result);
}

static pt::ptime CalculateSunEventTimestamp(dt::date date, double angle, bool sunrise) {
    double julianDate = GregorianToJulian(date);
    double julianCenturies = CalculateJulianCenturies(julianDate);
    double sunriseHourAngle = CalculateSunEventHourAngle(Config::Instance->latitude(), CalculateSolarDeclination(julianCenturies), angle, sunrise);
    double timeMinutes = 720 - 4 * (Config::Instance->longitude() + RadiansToDegrees(sunriseHourAngle)) - CalculateEquationOfTime(julianCenturies);

    double sunriseJulianCenturies = CalculateJulianCenturies(julianDate + timeMinutes / 1440.0);
    sunriseHourAngle = CalculateSunEventHourAngle(Config::Instance->latitude(), CalculateSolarDeclination(sunriseJulianCenturies), angle, sunrise);
    timeMinutes = 720 - 4 * (Config::Instance->longitude() + RadiansToDegrees(sunriseHourAngle)) - CalculateEquationOfTime(sunriseJulianCenturies);
    return pt::ptime(date, pt::time_duration(Utility::TimezoneOffset(), 0, static_cast<int>(std::round(timeMinutes * 60.0))));
}

pt::ptime Astronomy::CalculateSunrise(dt::date date) {
    return CalculateSunEventTimestamp(date, Config::Instance->sunriseAngle(), true);
}

pt::ptime Astronomy::CalculateSunset(dt::date date) {
    return CalculateSunEventTimestamp(date, Config::Instance->sunsetAngle(), false);
}

} // namespace cp
