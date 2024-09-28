#pragma once

// STL modules
#include <array>
#include <string>

// Custom modules
#include "common/i2c.hpp"
#include "common/utility.hpp"

namespace kc {

namespace Display
{
    namespace MasterConst
    {
        namespace Dimensions
        {
            constexpr int Rows = 2;
            constexpr int Columns = 16;
        }

        namespace Configurations
        {
            constexpr uint8_t RegisterSelect = 0b0001;
            constexpr uint8_t ReadWrite      = 0b0010;
            constexpr uint8_t Enable         = 0b0100;
            constexpr uint8_t Backlight      = 0b1000;
        }

        namespace Instructions
        {
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
    
    class Master : private I2C::Device
    {
    public:
        // Represents what's shown on the display
        using Screen = std::array<std::array<uint8_t, MasterConst::Dimensions::Columns>, MasterConst::Dimensions::Rows>;

        enum class CustomCharacter : char
        {
            HappyFace = 1,
            SadFace,
            UndefinedDot,
            Up,
            Down,
            UpArrow,
            DownArrow,
        };

    private:
        int m_row;
        int m_column;
        bool m_backlight;
        Screen m_screen;

    private:
        /// @brief Send byte to the display
        /// @param byte The byte to send
        /// @param instruction Whether the byte is an instruction or not
        void sendByte(uint8_t byte, bool instruction = false);

        /// @brief Pulse display's ENABLE pin
        /// @param pinConfiguration Current display's pin configuration
        void enable(uint8_t pinConfiguration);

        /// @brief Initialize custom characters
        void initCustomCharacters();

    public:
        /// @brief Initialize the display
        /// @param port I2C port the display is attached to
        /// @param backlight Whether to enable display's backlight or not
        Master(const std::string& port, bool backlight);

        ~Master();

        /// @brief Get cursor's position row
        /// @return Cursor's position row
        inline int row() const
        {
            return m_row;
        }

        /// @brief Get cursor's position column
        /// @return Cursor's position column
        inline int column() const
        {
            return m_column;
        }

        /// @brief Check if display's backlight is on
        /// @return True if display's backlight is on
        inline bool backlight() const
        {
            return m_backlight;
        }

        /// @brief Get what's shown on the display
        /// @return What's shown on the display
        inline Screen screen() const
        {
            return m_screen;
        }

        /// @brief Configure display
        /// @param on Whether the display should be on or not
        /// @param showCursor Whether the display should show cursor or not
        /// @param showBlinkingBlock Whether the display should show blinking block or not
        void configure(bool on, bool showCursor, bool showBlinkingBlock);

        /// @brief Configure display backlight
        /// @param enabled Whether the backlight should be enabled or not
        void backlight(bool enabled);

        /// @brief Clear display's screen and return cursor home
        void clear();

        /// @brief Return cursor home
        void home();

        /// @brief Set cursor position
        /// @param row Position row
        /// @param column Position column
        void position(int row, int column);

        /// @brief Print character
        /// @param character The character to print
        void print(char character);

        /// @brief Print character at position
        /// @param row Position row
        /// @param column Position column
        /// @param character The character to print
        void print(int row, int column, char character);

        /// @brief Print string
        /// @param string The string to print
        void print(const std::string& string);

        /// @brief Print string at position
        /// @param row Position row
        /// @param column Position column
        /// @param string The string to print
        void print(int row, int column, const std::string& string);

        /// @brief Print screen
        /// @param screen The screen to print
        void print(const Screen& screen);
    };
}

} // namespace kc
