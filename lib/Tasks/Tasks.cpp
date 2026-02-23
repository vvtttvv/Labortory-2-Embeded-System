#include "Tasks.h"
#include "Signals.h"
#include "Led.h"
#include "Keypad4x4.h"

#define LED1_PIN 13 // Task 1 — toggle LED  (green)
#define LED2_PIN 12 // Task 2 — blinking LED (yellow)
#define LED3_PIN 2  // Indicator LED (debugging)
#define KEYPAD_FIRST 4

static Led led1(LED1_PIN);
static Led led2(LED2_PIN);
static Led led3(LED3_PIN);
static Keypad4x4 keypad(KEYPAD_FIRST);

#define TASK2_REC_MS 50
static volatile uint16_t task2BlinkCnt = 0;

#define BLINK_STEP 100
#define BLINK_MIN 100
#define BLINK_MAX 2000

void Tasks::initHardware()
{
    led1.init();
    led2.init();
    led3.init();
    keypad.init();
}

void Tasks::keypadScan()
{
    char k = keypad.getKey();
    if (k != '\0')
    {
        sig_key = k;
    }
}

void Tasks::buttonLed()
{
    if (sig_key == '1')
    {
        sig_key = '\0';
        led1.toggle();
        sig_led1State = led1.getState();
    }
}

void Tasks::blinkLed()
{
    if (!sig_led1State)
    {
        // LED1 is OFF -> blink LED2 at the interval set by Task 3
        task2BlinkCnt += TASK2_REC_MS;
        if (task2BlinkCnt >= (uint16_t)sig_blinkInterval)
        {
            led2.toggle();
            sig_led2State = led2.getState();
            task2BlinkCnt = 0;
        }
    }
    else
    {
        // LED1 is ON -> keep LED2 OFF
        if (sig_led2State)
        {
            led2.off();
            sig_led2State = false;
        }
        task2BlinkCnt = 0;
    }
}

void Tasks::stateVariable()
{
    if (sig_key == '3')
    {
        sig_key = '\0';
        if (sig_blinkInterval + BLINK_STEP <= BLINK_MAX)
            sig_blinkInterval += BLINK_STEP;
    }
    if (sig_key == '2')
    {
        sig_key = '\0';
        if (sig_blinkInterval - BLINK_STEP >= BLINK_MIN)
            sig_blinkInterval -= BLINK_STEP;
    }
}
