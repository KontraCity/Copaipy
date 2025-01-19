#include "common/i2c.hpp"

#include <stdexcept>

#ifdef __unix__
    #include <unistd.h>
    #include <errno.h>

    #include <wiringPiI2C.h>
#endif

#include <fmt/format.h>

namespace cp {

I2C::Device::Device(const std::string& port, uint8_t address)
    : m_port(port)
    , m_address(address) {
#ifdef __unix__
    m_fd = wiringPiI2CSetupInterface(("/dev/" + m_port).c_str(), m_address);
    if (m_fd == -1) {
        throw std::runtime_error(fmt::format(
            "cp::I2C::Device::Device(): Couldn't initialize communication with I2C device [port: \"{}\", address: {:#x}, errno: {}]",
            m_port, m_address, errno
        ));
    }
#endif
}

I2C::Device::~Device() {
#ifdef __unix__
    close(m_fd);
#endif
}

void I2C::Device::send(const std::vector<uint8_t>& data) {
#ifdef __unix__
    int bytesSent = write(m_fd, data.data(), data.size());
    if (bytesSent == -1) {
        throw std::runtime_error(fmt::format(
            "cp::I2C::Device::send(): Couldn't send data to I2C device [port: \"{}\", address: {:#x}, errno: {}]",
            m_port, m_address, errno
        ));
    }
#endif
}

std::vector<uint8_t> I2C::Device::receive(size_t length) {
    std::vector<uint8_t> buffer(length);
#ifdef __unix__
    int bytesReceived = read(m_fd, buffer.data(), buffer.size());
    if (bytesReceived == -1) {
        throw std::runtime_error(fmt::format(
            "cp::I2C::Device::receive(): Couldn't receive data from I2C device [port: \"{}\", address: {:#x}, errno: {}]",
            m_port, m_address, errno
        ));
    }
#endif
    return buffer;
}

} // namespace cp
