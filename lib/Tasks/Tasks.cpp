#include "Tasks.h"
#include "Signals.h"
#include "Led.h"
#include "Button.h"
#include <Arduino.h>
#include <stdio.h>

#define LED_GREEN_PIN   13
#define LED_YELLOW_PIN  12
#define LED_RED_PIN      8
#define BTN_PIN          2

static Led    ledGreen(LED_GREEN_PIN);
static Led    ledYellow(LED_YELLOW_PIN);
static Led    ledRed(LED_RED_PIN);
static Button btn(BTN_PIN);

static uint8_t yellowBlinkRemaining = 0;

void Tasks::initHardware()
{
    ledGreen.init();
    ledYellow.init();
    ledRed.init();
    btn.init();
}

void Tasks::buttonMonitor()
{
    enum State { IDLE, PRESSED, LONG_PRESS };
    static State         state      = IDLE;
    static unsigned long pressStart = 0;

    bool currentState = btn.readRaw();

    switch (state)
    {
    case IDLE:
        if (currentState)
        {
            pressStart = millis();
            ledGreen.off();
            ledRed.off();
            state = PRESSED;
        }
        break;

    case PRESSED:
        if (!currentState)
        {
            sig_pressDuration = (uint16_t)(millis() - pressStart);
            sig_isLongPress   = false;
            ledGreen.on();
            sig_pressDetected = true; // signal Task 2
            state = IDLE;
        }
        else if (millis() - pressStart >= 500)
        {
            ledRed.on();
            state = LONG_PRESS;
        }
        break;

    case LONG_PRESS:
        if (!currentState)
        {
            sig_pressDuration = (uint16_t)(millis() - pressStart);
            sig_isLongPress   = true;
            sig_pressDetected = true;
            state = IDLE;
        }
        break;
    }
}

void Tasks::pressStats()
{
    if (sig_pressDetected)
    {
        sig_pressDetected = false;
        ledYellow.off();

        sig_totalPresses++;

        if (sig_isLongPress)
        {
            sig_longPresses++;
            sig_totalLongDuration += sig_pressDuration;
            yellowBlinkRemaining = 20; // 10 blinks = 20 toggles
        }
        else
        {
            sig_shortPresses++;
            sig_totalShortDuration += sig_pressDuration;
            yellowBlinkRemaining = 10; //  5 blinks = 10 toggles
        }
    }

    if (yellowBlinkRemaining > 0)
    {
        ledYellow.toggle();
        yellowBlinkRemaining--;
    }
    else
    {
        ledYellow.off();
    }
}

void Tasks::periodicReport()
{
    printf("Statistics for the last 10s\n");
    printf("Total apasari:           %u\n", sig_totalPresses);
    printf("Apasari scurte (<500ms): %u\n", sig_shortPresses);
    printf("Apasari lungi (>=500ms): %u\n", sig_longPresses);

    if (sig_totalPresses > 0)
    {
        uint32_t totalDuration = sig_totalShortDuration + sig_totalLongDuration;
        uint16_t avgDuration   = (uint16_t)(totalDuration / sig_totalPresses);
        printf("Durata medie:            %u ms\n", avgDuration);
    }
    else
    {
        printf("Durata medie:            0 ms\n");
    }

    sig_totalPresses       = 0;
    sig_shortPresses       = 0;
    sig_longPresses        = 0;
    sig_totalShortDuration = 0;
    sig_totalLongDuration  = 0;
}
