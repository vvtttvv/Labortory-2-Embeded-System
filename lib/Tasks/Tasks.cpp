#include "Tasks.h"
#include "Signals.h"
#include "Led.h"
#include "Button.h"
#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <stdio.h>

/* ── Pin mapping (same as bare-metal, Nano-compatible) ── */
#define LED_GREEN_PIN   13
#define LED_YELLOW_PIN  12
#define LED_RED_PIN      8
#define BTN_PIN          2

static Led    ledGreen(LED_GREEN_PIN);
static Led    ledYellow(LED_YELLOW_PIN);
static Led    ledRed(LED_RED_PIN);
static Button btn(BTN_PIN);

/* ──────────────────────────────────────────────────────── */
void Tasks::initHardware()
{
    ledGreen.init();
    ledYellow.init();
    ledRed.init();
    btn.init();
}

/* ══════════════════════════════════════════════════════════
   Task 1 — Button Monitor (period ≈ 10 ms)
   Detects press/release, measures duration, lights green
   (short < 500 ms) or red (long >= 500 ms) LED, then
   gives pressSemaphore to signal Task 2.
   ══════════════════════════════════════════════════════════ */
void Tasks::buttonMonitorTask(void *pvParameters)
{
    (void)pvParameters;

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
                /* Short press detected */
                TickType_t elapsed = now - pressStartTick;
                pressInfo.duration = (uint16_t)(elapsed * portTICK_PERIOD_MS);
                pressInfo.isLong   = false;
                ledGreen.on();
                ledRed.off();
                xSemaphoreGive(pressSemaphore);
                debounceStart = now;
                state = DEBOUNCE;
            }
            else if ((now - pressStartTick) >= pdMS_TO_TICKS(500))
            {
                ledRed.on();
                state = LONG_PRESS;
            }
            break;

        case LONG_PRESS:
            if (!pressed)
            {
                /* Long press detected — red stays on, green stays off */
                TickType_t elapsed = now - pressStartTick;
                pressInfo.duration = (uint16_t)(elapsed * portTICK_PERIOD_MS);
                pressInfo.isLong   = true;
                ledGreen.off();
                xSemaphoreGive(pressSemaphore);
                debounceStart = now;
                state = DEBOUNCE;
            }
            break;

        case DEBOUNCE:
            /* Ignore button for 50 ms after release to prevent bounce */
            if ((now - debounceStart) >= pdMS_TO_TICKS(50))
            {
                state = IDLE;
            }
            break;
        }

        /* Run every 10 ms using vTaskDelayUntil for precise periodicity */
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(10));
    }
}

/* ══════════════════════════════════════════════════════════
   Task 2 — Press Statistics & Yellow LED Blink
   Blocks on pressSemaphore (event from Task 1).
   Updates shared stats under mutex protection.
   Blinks yellow LED: 5 times for short, 10 times for long.
   ══════════════════════════════════════════════════════════ */
void Tasks::pressStatsTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        /* Block indefinitely until Task 1 signals a press */
        if (xSemaphoreTake(pressSemaphore, portMAX_DELAY) == pdTRUE)
        {
            /* Read press info (safe — Task 1 won't overwrite until next press) */
            uint16_t duration = pressInfo.duration;
            bool     isLong   = pressInfo.isLong;

            /* Update stats under mutex protection */
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

            /* Blink yellow LED: 5 blinks (short) or 10 blinks (long) */
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

/* ══════════════════════════════════════════════════════════
   Task 3 — Periodic Report (every 10 seconds)
   Reads stats under mutex, prints report via STDIO,
   then resets all counters.
   ══════════════════════════════════════════════════════════ */
void Tasks::periodicReportTask(void *pvParameters)
{
    (void)pvParameters;

    TickType_t xLastWake = xTaskGetTickCount();

    for (;;)
    {
        /* Wait exactly 10 seconds using vTaskDelayUntil */
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(10000));

        /* Take a snapshot of stats under mutex */
        PressStats snapshot;

        if (xSemaphoreTake(statsMutex, portMAX_DELAY) == pdTRUE)
        {
            snapshot = stats;

            /* Reset statistics */
            stats.totalPresses       = 0;
            stats.shortPresses       = 0;
            stats.longPresses        = 0;
            stats.totalShortDuration = 0;
            stats.totalLongDuration  = 0;

            xSemaphoreGive(statsMutex);
        }

        /* Print report using Serial (much lighter stack than printf on AVR) */
        Serial.println(F("=== Report (last 10s) ==="));
        Serial.print(F("Total presses:           ")); Serial.println(snapshot.totalPresses);
        Serial.print(F("Short presses (<500ms):  ")); Serial.println(snapshot.shortPresses);
        Serial.print(F("Long presses (>=500ms):  ")); Serial.println(snapshot.longPresses);

        if (snapshot.totalPresses > 0)
        {
            uint32_t totalDur = snapshot.totalShortDuration + snapshot.totalLongDuration;
            uint16_t avgDur   = (uint16_t)(totalDur / snapshot.totalPresses);
            Serial.print(F("Average duration:        ")); Serial.print(avgDur); Serial.println(F(" ms"));
        }
        else
        {
            Serial.println(F("Average duration:        0 ms"));
        }
        Serial.println(F("========================="));
    }
}
