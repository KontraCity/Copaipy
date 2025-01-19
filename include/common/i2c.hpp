#pragma once

#include <string>
#include <vector>

namespace cp {

namespace I2C {
    class Device {
    private:
        std::string m_port;
        uint8_t m_address;
        int m_fd;

    public:
        Device(const std::string& port, uint8_t address);

        ~Device();

    public:
        void send(const std::vector<uint8_t>& data);

        std::vector<uint8_t> receive(size_t length);

        template <typename Value>
        inline Value receiveValue(uint8_t firstByteRegister, bool reverse = false, size_t lengthLimit = -1) {
            send({ firstByteRegister });
            std::vector<uint8_t> response = receive(lengthLimit == -1 ? sizeof(Value) : lengthLimit);

            Value value = 0;
            if (reverse) {
                for (int index = 0, offset = 0; index < response.size(); ++index, offset += 8) {
                    value |= response[index] << offset;
                }
            }
            else {
                for (int index = static_cast<int>(response.size() - 1), offset = 0; index >= 0; --index, offset += 8) {
                    value |= response[index] << offset;
                }
            }
            return value;
        }
    };
}

} // namespace cp
