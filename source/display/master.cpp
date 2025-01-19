#include "display/master.hpp"
using namespace cp::Display::MasterConst;

#include "common/utility.hpp"

namespace cp {

Display::Master::Master(const std::string& port, bool backlight)
    : Device(port, 0x3F)
    , m_backlight(backlight) {
    sendByte(Instructions::SetAddress | 0b0000'0011, true);
    sendByte(Instructions::SetAddress | 0b0000'0010, true);
    sendByte(Instructions::FunctionSet | 0b0000'1000, true);
    sendByte(Instructions::EntryModeSet | 0b0000'0010, true);
    initCustomCharacters();
    clear();
    configure(true, false, false);
}

Display::Master::~Master() {
    configure(false, false, false);
    clear();
}

void Display::Master::sendByte(uint8_t byte, bool instruction) {
    uint8_t pinConfiguration = byte & 0b1111'0000;
    if (!instruction) {
        pinConfiguration |= Configurations::RegisterSelect;
    }
    if (m_backlight) {
        pinConfiguration |= Configurations::Backlight;
    }

    send({ pinConfiguration });
    enable(pinConfiguration);

    pinConfiguration = (byte << 4 & 0b1111'0000) | (pinConfiguration & 0b0000'1111);
    send({ pinConfiguration });
    enable(pinConfiguration);
}

void Display::Master::enable(uint8_t pinConfiguration) {
    send({ static_cast<uint8_t>(pinConfiguration | Configurations::Enable) });
    Utility::Sleep(0.0005);
    send({ pinConfiguration });
    Utility::Sleep(0.0005);
}

void Display::Master::initCustomCharacters() {
    sendByte(Instructions::SetAddress | static_cast<uint8_t>(CustomCharacter::HappyFace) << 3, true);
    sendByte(0b00000);
    sendByte(0b01010);
    sendByte(0b01010);
    sendByte(0b01010);
    sendByte(0b00000);
    sendByte(0b10001);
    sendByte(0b01110);
    sendByte(0b00000);

    sendByte(Instructions::SetAddress | static_cast<uint8_t>(CustomCharacter::SadFace) << 3, true);
    sendByte(0b01010);
    sendByte(0b10001);
    sendByte(0b01010);
    sendByte(0b01010);
    sendByte(0b00000);
    sendByte(0b01110);
    sendByte(0b10001);
    sendByte(0b00000);

    sendByte(Instructions::SetAddress | static_cast<uint8_t>(CustomCharacter::UndefinedDot) << 3, true);
    sendByte(0b00000);
    sendByte(0b00000);
    sendByte(0b01010);
    sendByte(0b00100);
    sendByte(0b01010);
    sendByte(0b00000);
    sendByte(0b00000);
    sendByte(0b00000);

    sendByte(Instructions::SetAddress | static_cast<uint8_t>(CustomCharacter::Up) << 3, true);
    sendByte(0b00000);
    sendByte(0b00000);
    sendByte(0b00100);
    sendByte(0b01010);
    sendByte(0b10001);
    sendByte(0b00000);
    sendByte(0b00000);
    sendByte(0b00000);

    sendByte(Instructions::SetAddress | static_cast<uint8_t>(CustomCharacter::Down) << 3, true);
    sendByte(0b00000);
    sendByte(0b00000);
    sendByte(0b10001);
    sendByte(0b01010);
    sendByte(0b00100);
    sendByte(0b00000);
    sendByte(0b00000);
    sendByte(0b00000);

    sendByte(Instructions::SetAddress | static_cast<uint8_t>(CustomCharacter::UpArrow) << 3, true);
    sendByte(0b00000);
    sendByte(0b00100);
    sendByte(0b01110);
    sendByte(0b10101);
    sendByte(0b00100);
    sendByte(0b00100);
    sendByte(0b00100);
    sendByte(0b00000);

    sendByte(Instructions::SetAddress | static_cast<uint8_t>(CustomCharacter::DownArrow) << 3, true);
    sendByte(0b00000);
    sendByte(0b00100);
    sendByte(0b00100);
    sendByte(0b00100);
    sendByte(0b10101);
    sendByte(0b01110);
    sendByte(0b00100);
    sendByte(0b00000);
}

void Display::Master::configure(bool on, bool showCursor, bool showBlinkingBlock) {
    uint8_t instruction = Instructions::DisplayControl;
    instruction |= static_cast<uint8_t>(on << 2);
    instruction |= static_cast<uint8_t>(showCursor << 1);
    instruction |= static_cast<uint8_t>(showBlinkingBlock);
    sendByte(instruction, true);
}

void Display::Master::backlight(bool enabled) {
    if (m_backlight != enabled) {
        send({ enabled ? Configurations::Backlight : static_cast<uint8_t>(0) });
        m_backlight = enabled;
    }
}

void Display::Master::clear() {
    sendByte(Instructions::ClearDisplay, true);
    m_row = 0;
    m_column = 0;

    /* Empty screen = screen filled with spaces */
    m_screen[0].fill(' ');
    m_screen[1].fill(' ');
}

void Display::Master::home() {
    sendByte(Instructions::ReturnHome, true);
    m_row = 0;
    m_column = 0;
}

void Display::Master::position(int row, int column) {
    m_row = static_cast<int>(Utility::Limit(row, 0, Dimensions::Rows - 1));
    m_column = static_cast<int>(Utility::Limit(column, 0, Dimensions::Columns));

    uint8_t instruction = Instructions::SetPosition;
    instruction |= static_cast<uint8_t>(m_row << 6);
    instruction |= static_cast<uint8_t>(m_column);
    sendByte(instruction, true);
}

void Display::Master::print(char character) {
    if (m_column < Dimensions::Columns) {
        sendByte(character);
        m_screen[m_row][m_column++] = character;
    }
}

void Display::Master::print(int row, int column, char character) {
    position(row, column);
    print(character);
}

void Display::Master::print(const std::string& string) {
    bool skipping = false;
    for (size_t index = 0, length = string.length(); index < length && m_column < Dimensions::Columns; ++index, ++m_column) {
        if (m_screen[m_row][m_column] == string[index]) {
            skipping = true;
            continue;
        }

        if (skipping) {
            skipping = false;
            position(m_row, m_column);
        }

        sendByte(string[index]);
        m_screen[m_row][m_column] = string[index];
    }

    if (skipping) {
        position(m_row, m_column);
    }
}

void Display::Master::print(int row, int column, const std::string& string) {
    position(row, column);
    print(string);
}

void Display::Master::print(const Screen& screen) {
    print(0, 0, std::string(screen[0].begin(), screen[0].end()));
    print(1, 0, std::string(screen[1].begin(), screen[1].end()));
}

} // namespace cp
