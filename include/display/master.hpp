#pragma once

#include <array>
#include <string>

#include "common/i2c.hpp"

namespace cp {

namespace Display {
    namespace MasterConst {
        namespace Dimensions {
            constexpr int Rows = 2;
            constexpr int Columns = 16;
        }

        namespace Configurations {
            constexpr uint8_t RegisterSelect = 0b0001;
            constexpr uint8_t ReadWrite      = 0b0010;
            constexpr uint8_t Enable         = 0b0100;
            constexpr uint8_t Backlight      = 0b1000;
        }

        namespace Instructions {
            constexpr uint8_t ClearDisplay   = 0b0000'0001;
            constexpr uint8_t ReturnHome     = 0b0000'0010;
            constexpr uint8_t EntryModeSet   = 0b0000'0100;
            constexpr uint8_t DisplayControl = 0b0000'1000;
            constexpr uint8_t Shift          = 0b0001'0000;
            constexpr uint8_t FunctionSet    = 0b0010'0000;
            constexpr uint8_t SetAddress     = 0b0100'0000;
            constexpr uint8_t SetPosition    = 0b1000'0000;
        }
    }
    
    class Master : private I2C::Device {
    public:
        using Screen = std::array<std::array<uint8_t, MasterConst::Dimensions::Columns>, MasterConst::Dimensions::Rows>;

        enum class CustomCharacter : char {
            HappyFace = 1,
            SadFace,
            UndefinedDot,
            Up,
            Down,
            UpArrow,
            DownArrow,
        };

    private:
        int m_row = 0;
        int m_column = 0;
        bool m_backlight = true;
        Screen m_screen = {};

    public:
        Master(const std::string& port, bool backlight = true);

        ~Master();

    private:
        void sendByte(uint8_t byte, bool instruction = false);

        void enable(uint8_t pinConfiguration);

        void initCustomCharacters();

    public:
        void configure(bool on, bool showCursor, bool showBlinkingBlock);

        void backlight(bool enabled);

        void clear();

        void home();

        void position(int row, int column);

        void print(char character);

        void print(int row, int column, char character);

        void print(const std::string& string);

        void print(int row, int column, const std::string& string);

        void print(const Screen& screen);

    public:
        inline int row() const {
            return m_row;
        }

        inline int column() const {
            return m_column;
        }

        inline bool backlight() const {
            return m_backlight;
        }

        inline Screen screen() const {
            return m_screen;
        }
    };
}

} // namespace cp
