#include "Keypad4x4.h"

const char Keypad4x4::_keyMap[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};

Keypad4x4::Keypad4x4(uint8_t firstPin) : _lastKey('\0')
{
    for (uint8_t i = 0; i < KEYPAD_ROWS; i++)
        _rowPins[i] = firstPin + i; // R1..R4
    for (uint8_t i = 0; i < KEYPAD_COLS; i++)
        _colPins[i] = firstPin + KEYPAD_ROWS + i; // C1..C4
}

void Keypad4x4::init()
{
    // Rows: default to INPUT_PULLUP (idle high)
    for (uint8_t i = 0; i < KEYPAD_ROWS; i++)
    {
        pinMode(_rowPins[i], INPUT_PULLUP);
    }
    // Columns: INPUT_PULLUP
    for (uint8_t i = 0; i < KEYPAD_COLS; i++)
    {
        pinMode(_colPins[i], INPUT_PULLUP);
    }
}

char Keypad4x4::scanRaw()
{
    for (uint8_t r = 0; r < KEYPAD_ROWS; r++)
    {
        // Drive this row LOW
        pinMode(_rowPins[r], OUTPUT);
        digitalWrite(_rowPins[r], LOW);

        // Read each column
        for (uint8_t c = 0; c < KEYPAD_COLS; c++)
        {
            if (digitalRead(_colPins[c]) == LOW)
            {
                // Restore row to idle before returning
                pinMode(_rowPins[r], INPUT_PULLUP);
                return _keyMap[r][c];
            }
        }

        // Restore row to idle
        pinMode(_rowPins[r], INPUT_PULLUP);
    }
    return '\0'; // No key pressed
}

char Keypad4x4::getKey()
{
    char key = scanRaw();

    if (key != '\0' && key != _lastKey)
    {
        _lastKey = key;
        return key; // New press - return it once
    }

    if (key == '\0')
    {
        _lastKey = '\0'; // Key released - reset
    }

    return '\0'; // No new press
}
