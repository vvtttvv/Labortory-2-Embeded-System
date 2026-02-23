#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

class Button
{
public:
    Button(uint8_t pin);

    void init();

    bool isPressed();

    bool readRaw() const;

private:
    uint8_t _pin;
    bool _prevPressed;
};

#endif
