#include "SerialCmd.h"
#include "Signals.h"
#include <Arduino.h>
#include <stdio.h>

#define BLINK_STEP 100
#define BLINK_MIN 100
#define BLINK_MAX 2000

void SerialCmd::poll()
{
    if (!Serial.available())
        return;

    char cmd;
    scanf(" %c", &cmd);
    printf("[IN] cmd='%c'\n", cmd);

    switch (cmd)
    {
    case 'r':
    case 'R':
        sig_blinkInterval = 500;
        printf("[IN] BlinkInterval reset to 500ms\n");
        break;
    case '+':
        if (sig_blinkInterval + BLINK_STEP <= BLINK_MAX)
            sig_blinkInterval += BLINK_STEP;
        printf("[IN] BlinkInterval = %dms\n", sig_blinkInterval);
        break;
    case '-':
        if (sig_blinkInterval - BLINK_STEP >= BLINK_MIN)
            sig_blinkInterval -= BLINK_STEP;
        printf("[IN] BlinkInterval = %dms\n", sig_blinkInterval);
        break;
    default:
        printf("[IN] Unknown command. Use: r/+/-\n");
        break;
    }
}

void SerialCmd::idleReport()
{
    static unsigned long lastReport = 0;
    unsigned long now = millis();

    if (now - lastReport >= 1000)
    {
        printf("[IDLE] LED1=%s | LED2=%s | Blink=%dms\n",
               sig_led1State ? "ON " : "OFF",
               sig_led2State ? "ON " : "OFF",
               sig_blinkInterval);
        lastReport = now;
    }
}
