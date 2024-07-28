#pragma once

// STL modules
#include <string>
#include <vector>
#include <stdexcept>

#ifdef __unix__
    // UNIX modules
    #include <unistd.h>
    #include <errno.h>

    // Library WiringPi
    #include <wiringPiI2C.h>
#endif

// Library {fmt}
#include <fmt/format.h>

namespace kc {

namespace I2C
{
    class Device
    {
    private:
        std::string m_port;
        uint8_t m_address;
        int m_fd;

    public:
        /// @brief Initialize communication with I2C device
        /// @param port I2C port to use
        /// @param address I2C device address
        /// @throw std::runtime_error if internal error occurs
        Device(const std::string& port, uint8_t address);

        ~Device();

        /// @brief Send data to I2C device
        /// @param data The data to send
        /// @throw std::runtime_error if internal error occurs
        void send(const std::vector<uint8_t>& data);

        /// @brief Receive data from I2C device
        /// @param length Length of the data to receive
        /// @throw std::runtime_error if internal error occurs
        /// @return Received data
        std::vector<uint8_t> receive(size_t length);

        /// @brief Receive value from I2C device
        /// @tparam Value Type of value to receive
        /// @param firstByteRegister First value byte register
        /// @param reverse Whether to receive data in reverse or not
        /// @param lengthLimit Value bytes length limit
        /// @throw std::runtime_error if internal error occurs
        /// @return Received value
        template <typename Value>
        Value receiveValue(uint8_t firstByteRegister, bool reverse = false, size_t lengthLimit = -1)
        {
            send({ firstByteRegister });
            std::vector<uint8_t> response = receive(lengthLimit == -1 ? sizeof(Value) : lengthLimit);

            Value value = 0;
            if (reverse)
                for (int index = 0, offset = 0; index < response.size(); ++index, offset += 8)
                    value |= response[index] << offset;
            else
                for (int index = static_cast<int>(response.size() - 1), offset = 0; index >= 0; --index, offset += 8)
                    value |= response[index] << offset;
            return value;
        }
    };
}

} // namespace kc
