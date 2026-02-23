#include "Led.h"

Led::Led(uint8_t pin) : _pin(pin), _state(false) {}

void Led::init()
{
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    _state = false;
}

void Led::on()
{
    digitalWrite(_pin, HIGH);
    _state = true;
}

void Led::off()
{
    digitalWrite(_pin, LOW);
    _state = false;
}

void Led::toggle()
{
    _state = !_state;
    digitalWrite(_pin, _state ? HIGH : LOW);
}

bool Led::getState() const
{
    return _state;
}
