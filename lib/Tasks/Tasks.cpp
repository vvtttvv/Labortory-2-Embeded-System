#include "Tasks.h"
#include "Signals.h"
#include "Led.h"
#include "Button.h"
#include <Arduino.h>
#include <stdio.h>

// ---- Pin definitions (match diagram.json) ----
#define LED_GREEN_PIN   13   // led8  — short press indicator
#define LED_YELLOW_PIN  12   // led7  — blink counter
#define LED_RED_PIN      8   // led1  — long press indicator
#define BTN_PIN          2   // btn2  — single button

static Led    ledGreen(LED_GREEN_PIN);
static Led    ledYellow(LED_YELLOW_PIN);
static Led    ledRed(LED_RED_PIN);
static Button btn(BTN_PIN);

// Task 2 internal state
static uint8_t yellowBlinkRemaining = 0;

// --------------- initHardware -----------------
void Tasks::initHardware()
{
    ledGreen.init();
    ledYellow.init();
    ledRed.init();
    btn.init();
}

// ============ Task 1: buttonMonitor  rec=10 ms  off=0 ms ============
// State-machine (no busy-wait):
//   IDLE     -> PRESSED  (on press edge)
//   PRESSED  -> IDLE     (on release — short < 500 ms)
//   PRESSED  -> LONG     (while held & elapsed >= 500 ms, red LED on)
//   LONG     -> IDLE     (on release)
void Tasks::buttonMonitor()
{
    enum State { IDLE, PRESSED, LONG_PRESS };
    static State         state      = IDLE;
    static unsigned long pressStart = 0;

    bool currentState = btn.readRaw();          // true = pressed (active LOW)

    switch (state)
    {
    case IDLE:
        if (currentState)
        {
            // ---- button just PRESSED ----
            pressStart = millis();
            ledGreen.off();
            ledRed.off();
            state = PRESSED;
        }
        break;

    case PRESSED:
        if (!currentState)
        {
            // ---- released before 500 ms => SHORT press ----
            sig_pressDuration = (uint16_t)(millis() - pressStart);
            sig_isLongPress   = false;
            ledGreen.on();              // green = short
            sig_pressDetected = true;   // signal Task 2
            state = IDLE;
        }
        else if (millis() - pressStart >= 500)
        {
            // ---- still held >= 500 ms => switch to LONG ----
            ledRed.on();                // red ON while held
            state = LONG_PRESS;
        }
        break;

    case LONG_PRESS:
        if (!currentState)
        {
            // ---- released after >= 500 ms => LONG press ----
            sig_pressDuration = (uint16_t)(millis() - pressStart);
            sig_isLongPress   = true;
            sig_pressDetected = true;   // signal Task 2
            state = IDLE;
        }
        break;
    }
}

// ============ Task 2: pressStats  rec=50 ms  off=5 ms ============
// Updates counters / sums and generates a rapid yellow-LED blink
// (5 blinks for short, 10 blinks for long).
void Tasks::pressStats()
{
    if (sig_pressDetected)
    {
        sig_pressDetected = false;
        ledYellow.off();               // reset to known state

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

    // Handle yellow LED blinking (one toggle per call = 50 ms step)
    if (yellowBlinkRemaining > 0)
    {
        ledYellow.toggle();
        yellowBlinkRemaining--;
    }
    else
    {
        ledYellow.off();               // ensure LED off when done
    }
}

// ============ Task 3: periodicReport  rec=10000 ms  off=15 ms ============
// Prints statistics via STDIO and resets counters.
void Tasks::periodicReport()
{
    printf("=== Raport (10s) ===\n");
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

    // Reset statistics
    sig_totalPresses       = 0;
    sig_shortPresses       = 0;
    sig_longPresses        = 0;
    sig_totalShortDuration = 0;
    sig_totalLongDuration  = 0;

    printf("Statistici resetate.\n");
    printf("====================\n");
}
