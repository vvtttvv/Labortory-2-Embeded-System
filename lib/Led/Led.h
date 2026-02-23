#ifndef LED_H
#define LED_H

#include <Arduino.h>

class Led
{
public:
    Led(uint8_t pin);

    void init();
    void on();
    void off();
    void toggle();
    bool getState() const;

private:
    uint8_t _pin;
    bool _state;
};

#endif
