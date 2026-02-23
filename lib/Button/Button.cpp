#include "Button.h"

Button::Button(uint8_t pin) : _pin(pin), _prevPressed(false) {}

void Button::init()
{
    pinMode(_pin, INPUT_PULLUP);
    _prevPressed = false;
}

bool Button::isPressed()
{
    bool pressed = (digitalRead(_pin) == LOW); // Active LOW (pull-up)

    if (pressed && !_prevPressed)
    {
        _prevPressed = pressed;
        return true; // New press detected
    }

    _prevPressed = pressed;
    return false;
}

bool Button::readRaw() const
{
    return (digitalRead(_pin) == LOW);
}
