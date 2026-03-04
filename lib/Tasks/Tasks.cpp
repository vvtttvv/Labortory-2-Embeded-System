#include "Tasks.h"
#include "Signals.h"
#include "Led.h"
#include "Button.h"
#include "UartStdio.h"
#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>

#define LED_GREEN_PIN   13
#define LED_YELLOW_PIN  12
#define LED_RED_PIN      8
#define BTN_PIN          2

static Led    ledGreen(LED_GREEN_PIN);
static Led    ledYellow(LED_YELLOW_PIN);
static Led    ledRed(LED_RED_PIN);
static Button btn(BTN_PIN);

void Tasks::initHardware()
{
    ledGreen.init();
    ledYellow.init();
    ledRed.init();
    btn.init();
}

bool Tasks::createAll()
{
    BaseType_t r1 = xTaskCreate(buttonMonitorTask,
                                "BtnMon", 256, NULL, 2, NULL);

    BaseType_t r2 = xTaskCreate(pressStatsTask,
                                "Stats",  256, NULL, 2, NULL);

    BaseType_t r3 = xTaskCreate(periodicReportTask,
                                "Report", 384, NULL, 1, NULL);

    return (r1 == pdPASS && r2 == pdPASS && r3 == pdPASS);
}

void Tasks::buttonMonitorTask(void *pvParameters)
{
    (void)pvParameters;

    UartStdio::println(F("[T1] Button monitor task started"));

    enum State { IDLE, PRESSED, LONG_PRESS, DEBOUNCE };
    State      state          = IDLE;
    TickType_t pressStartTick = 0;
    TickType_t debounceStart  = 0;

    TickType_t xLastWake = xTaskGetTickCount();

    for (;;)
    {
        bool pressed = btn.readRaw();
        TickType_t now = xTaskGetTickCount();

        switch (state)
        {
        case IDLE:
            if (pressed)
            {
                pressStartTick = now;
                ledGreen.off();
                ledRed.off();
                state = PRESSED;
            }
            break;

        case PRESSED:
            if (!pressed)
            {
                // Short press detected (< 500 ms)
                TickType_t elapsed = now - pressStartTick;
                pressInfo.duration = (uint16_t)(elapsed * portTICK_PERIOD_MS);
                pressInfo.isLong   = false;
                ledGreen.on();
                ledRed.off();
                UartStdio::println(F("[T1] Short press detected"));
                xSemaphoreGive(pressSemaphore);
                debounceStart = now;
                state = DEBOUNCE;
            }
            else if ((now - pressStartTick) >= pdMS_TO_TICKS(500))
            {
                // Holding for 500+ ms - switch red LED on
                ledRed.on();
                state = LONG_PRESS;
            }
            break;

        case LONG_PRESS:
            if (!pressed)
            {
                // Long press released (>= 500 ms) - red stays on
                TickType_t elapsed = now - pressStartTick;
                pressInfo.duration = (uint16_t)(elapsed * portTICK_PERIOD_MS);
                pressInfo.isLong   = true;
                UartStdio::println(F("[T1] Long press detected"));
                xSemaphoreGive(pressSemaphore);
                debounceStart = now;
                state = DEBOUNCE;
            }
            break;

        case DEBOUNCE:
            // Ignore button for ~200 ms after release to prevent bounce
            if ((now - debounceStart) >= pdMS_TO_TICKS(200))
            {
                state = IDLE;
            }
            break;
        }

        // I use vTaskDelayUntil instead of vTaskDelay, so the loop runs every ~10 ms regardless of how long the processing takes
        vTaskDelayUntil(&xLastWake, 1);
    }
}

void Tasks::pressStatsTask(void *pvParameters)
{
    (void)pvParameters;

    UartStdio::println(F("[T2] Press stats task started"));

    for (;;)
    {
        // Block until Task 1 signals a completed press
        if (xSemaphoreTake(pressSemaphore, portMAX_DELAY) == pdTRUE)
        {
            UartStdio::println(F("[T2] Semaphore received, processing"));
            // Read press info (safe - Task 1 won't overwrite until next press)
            uint16_t duration = pressInfo.duration;
            bool     isLong   = pressInfo.isLong;

            // Update stats under mutex protection
            if (xSemaphoreTake(statsMutex, portMAX_DELAY) == pdTRUE)
            {
                stats.totalPresses++;

                if (isLong)
                {
                    stats.longPresses++;
                    stats.totalLongDuration += duration;
                }
                else
                {
                    stats.shortPresses++;
                    stats.totalShortDuration += duration;
                }

                xSemaphoreGive(statsMutex);
            }

            // Blink yellow LED: 5 (short) or 10 (long) each blink = 50ms ON + 50ms OFF using vTaskDelay
            uint8_t blinks = isLong ? 10 : 5;
            for (uint8_t i = 0; i < blinks; i++)
            {
                ledYellow.on();
                vTaskDelay(pdMS_TO_TICKS(50));
                ledYellow.off();
                vTaskDelay(pdMS_TO_TICKS(50));
            }
        }
    }
}

void Tasks::periodicReportTask(void *pvParameters)
{
    (void)pvParameters;

    UartStdio::println(F("[T3] Periodic report task started"));

    TickType_t xLastWake = xTaskGetTickCount();

    for (;;)
    {
        // Wait exactly 10 seconds using vTaskDelayUntil
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(10000));

        UartStdio::println(F("[T3] 10s elapsed, generating report"));

        // Take a snapshot of stats under mutex, then reset
        PressStats snapshot = {0, 0, 0, 0, 0};

        if (xSemaphoreTake(statsMutex, portMAX_DELAY) == pdTRUE)
        {
            snapshot = stats;

            stats.totalPresses       = 0;
            stats.shortPresses       = 0;
            stats.longPresses        = 0;
            stats.totalShortDuration = 0;
            stats.totalLongDuration  = 0;

            xSemaphoreGive(statsMutex);
        }

        // Print report via stdio (printf)
        UartStdio::println(F("=== Report (last 10s) ==="));
        UartStdio::printf_P(PSTR("Total presses:           %u\n"), snapshot.totalPresses);
        UartStdio::printf_P(PSTR("Short presses (<500ms):  %u\n"), snapshot.shortPresses);
        UartStdio::printf_P(PSTR("Long presses (>=500ms):  %u\n"), snapshot.longPresses);

        if (snapshot.totalPresses > 0)
        {
            uint32_t totalDur = snapshot.totalShortDuration + snapshot.totalLongDuration;
            uint16_t avgDur   = (uint16_t)(totalDur / snapshot.totalPresses);
            UartStdio::printf_P(PSTR("Average duration:        %u ms\n"), avgDur);
        }
        else
        {
            UartStdio::println(F("Average duration:        0 ms"));
        }
        UartStdio::println(F("========================="));
    }
}
