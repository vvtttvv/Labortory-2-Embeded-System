#ifndef KEYPAD4X4_H
#define KEYPAD4X4_H

#include <Arduino.h>

#define KEYPAD_ROWS 4
#define KEYPAD_COLS 4

class Keypad4x4
{
public:
    Keypad4x4(uint8_t firstPin);

    void init();
    char getKey();

private:
    uint8_t _rowPins[KEYPAD_ROWS];
    uint8_t _colPins[KEYPAD_COLS];
    char _lastKey;

    static const char _keyMap[KEYPAD_ROWS][KEYPAD_COLS];

    char scanRaw();
};

#endif
