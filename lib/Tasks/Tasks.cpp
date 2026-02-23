#include "Tasks.h"
#include "Signals.h"
#include "Led.h"
#include "Button.h"

#define LED1_PIN 13
#define LED2_PIN 12
#define BTN_TOGGLE_PIN 8
#define BTN_DEC_PIN 7
#define BTN_INC_PIN 2

static Led led1(LED1_PIN);
static Led led2(LED2_PIN);
static Button btnToggle(BTN_TOGGLE_PIN);
static Button btnDec(BTN_DEC_PIN);
static Button btnInc(BTN_INC_PIN);

#define TASK2_REC_MS 50
static volatile uint16_t task2BlinkCnt = 0;

#define BLINK_STEP 100
#define BLINK_MIN 100
#define BLINK_MAX 2000

void Tasks::initHardware()
{
    led1.init();
    led2.init();
    btnToggle.init();
    btnDec.init();
    btnInc.init();
}

void Tasks::buttonScan()
{
    if (btnToggle.isPressed())
        sig_btnToggle = true;

    if (btnDec.isPressed())
        sig_btnDec = true;

    if (btnInc.isPressed())
        sig_btnInc = true;
}

void Tasks::buttonLed()
{
    if (sig_btnToggle)
    {
        sig_btnToggle = false; // consume signal
        led1.toggle();
        sig_led1State = led1.getState();
    }
}

void Tasks::blinkLed()
{
    if (!sig_led1State)
    {
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
    if (sig_btnInc)
    {
        sig_btnInc = false;
        if (sig_blinkInterval + BLINK_STEP <= BLINK_MAX)
            sig_blinkInterval += BLINK_STEP;
    }
    if (sig_btnDec)
    {
        sig_btnDec = false;
        if (sig_blinkInterval - BLINK_STEP >= BLINK_MIN)
            sig_blinkInterval -= BLINK_STEP;
    }
}
