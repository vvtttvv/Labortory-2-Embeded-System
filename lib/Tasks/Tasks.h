#ifndef TASKS_H
#define TASKS_H

namespace Tasks
{
    void initHardware();
    void buttonScan();    // Task 0 — scan 3 buttons (provider)
    void buttonLed();     // Task 1 — toggle LED1 on button press
    void blinkLed();      // Task 2 — blink LED2 when LED1 is OFF
    void stateVariable(); // Task 3 — adjust blink interval
}

#endif
