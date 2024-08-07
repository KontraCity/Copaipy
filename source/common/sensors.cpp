#include "common/sensors.hpp"

namespace kc {

/// @brief Perform AHT20 sensor measurement
/// @param device I2C AHT20 device to measure
/// @return Sensor measurement
static Sensors::Aht20Measurement MeasureAht20(I2C::Device& device)
{
    device.send({ 0xAC, 0x33, 0x00 });
    Utility::Sleep(0.08);

    std::vector<uint8_t> response = device.receive(7);
    return {
        Utility::Round((((response[3] & 0b0000'1111) << 16) | (response[4] << 8) | response[5]) / std::pow(2, 20) * 200 - 50, 2),
        Utility::Round(((response[1] << 12) | (response[2] << 4) | ((response[3] & 0b1111'0000) >> 4)) / std::pow(2, 20) * 100, 2)
    };
}

/// @brief Perform BMP280 sensor measurement
/// @param device I2C BMP280 device to measure
/// @return Sensor measurement
static Sensors::Bmp280Measurement MeasureBmp280(I2C::Device& device)
{
    device.send({ 0xF4, 0b111'010'01 });
    Utility::Sleep(0.05);

    uint16_t calibrationValue1 = device.receiveValue<uint16_t>(0x88, true);
    int16_t calibrationValue2 = device.receiveValue<int16_t>(0x8A, true);
    int16_t calibrationValue3 = device.receiveValue<int16_t>(0x8C, true);
    int32_t rawTemperature = device.receiveValue<int32_t>(0xFA, false, 3) >> 4;

    double var1 = (rawTemperature / 16384.0 - calibrationValue1 / 1024.0) * calibrationValue2;
    double var2 = (rawTemperature / 131072.0 - calibrationValue1 / 8192.0) * (rawTemperature / 131072.0 - calibrationValue1 / 8192.0) * calibrationValue3;
    double fineTemperature = var1 + var2;

    calibrationValue1 = device.receiveValue<uint16_t>(0x8E, true);
    calibrationValue2 = device.receiveValue<int16_t>(0x90, true);
    calibrationValue3 = device.receiveValue<int16_t>(0x92, true);
    int16_t calibrationValue4 = device.receiveValue<int16_t>(0x94, true);
    int16_t calibrationValue5 = device.receiveValue<int16_t>(0x96, true);
    int16_t calibrationValue6 = device.receiveValue<int16_t>(0x98, true);
    int16_t calibrationValue7 = device.receiveValue<int16_t>(0x9A, true);
    int16_t calibrationValue8 = device.receiveValue<int16_t>(0x9C, true);
    int16_t calibrationValue9 = device.receiveValue<int16_t>(0x9E, true);
    int32_t rawPressure = device.receiveValue<int32_t>(0xF7, false, 3) >> 4;

    var1 = fineTemperature / 2.0 - 64000.0;
    var2 = var1 * var1 * calibrationValue6 / 32768.0;
    var2 = var2 + var1 * calibrationValue5 * 2.0;
    var2 = (var2 / 4.0) + (calibrationValue4 * 65536.0);
    var1 = (calibrationValue3 * var1 * var1 / 524288.0 + calibrationValue2 * var1) / 524288.0;
    var1 = (1.0 + var1 / 32768.0) * calibrationValue1;
    double pressure = 1048576.0 - rawPressure;
    pressure = (pressure - (var2 / 4096.0)) * 6250.0 / var1;
    var1 = calibrationValue9 * pressure * pressure / 2147483648.0;
    var2 = pressure * calibrationValue8 / 32768.0;
    pressure = pressure + (var1 + var2 + calibrationValue7) / 16.0;

    return { Utility::Round(fineTemperature / 5120.0, 2), Utility::Round(pressure / 100.0, 2) };
}

Sensors::Measurement Sensors::Measure(Config::Pointer config, Location location)
{
#ifdef __unix__
    static std::mutex mutex;
    std::lock_guard lock(mutex);

    static I2C::Device externalAht20(config->externalPort(), 0x38);
    static I2C::Device externalBmp280(config->externalPort(), 0x77);
    static I2C::Device internalAht20(config->internalPort(), 0x38);
    static I2C::Device internalBmp280(config->internalPort(), 0x77);
    static bool initialized = false;
    if (!initialized)
    {
        // AHT20s initialization
        externalAht20.send({ 0xBE, 0x08, 0x00 });
        internalAht20.send({ 0xBE, 0x08, 0x00 });
        Utility::Sleep(0.01);

        // BMP280s initialization
        externalBmp280.send({ 0xB6 });
        internalBmp280.send({ 0xB6 });
        Utility::Sleep(0.002);

        initialized = true;
    }

    switch (location)
    {
        default:
        case Location::External:
            return { MeasureAht20(externalAht20), MeasureBmp280(externalBmp280) };
        case Location::Internal:
            return { MeasureAht20(internalAht20), MeasureBmp280(internalBmp280) };
    }
#else
    return { { 0.0, 0.0 }, { 0.0, 0.0 } };
#endif
}

} // namespace kc
