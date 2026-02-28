#include "Tasks.h"
#include "Signals.h"
#include "Led.h"
#include "Button.h"
#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>

/* ── Pin mapping (Nano-compatible) ── */
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
   Task 1 — Button Monitor (period ≈ 10 ms via vTaskDelayUntil)
   Detects press/release transitions, measures duration using
   xTaskGetTickCount(), lights green LED (short < 500 ms) or
   red LED (long >= 500 ms), then gives pressSemaphore to
   signal Task 2.
   ══════════════════════════════════════════════════════════ */
void Tasks::buttonMonitorTask(void *pvParameters)
{
    (void)pvParameters;

    Serial.println(F("[T1] Button monitor task started"));

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
                /* Short press detected (< 500 ms) */
                TickType_t elapsed = now - pressStartTick;
                pressInfo.duration = (uint16_t)(elapsed * portTICK_PERIOD_MS);
                pressInfo.isLong   = false;
                ledGreen.on();
                ledRed.off();
                Serial.println(F("[T1] Short press detected"));
                xSemaphoreGive(pressSemaphore);
                debounceStart = now;
                state = DEBOUNCE;
            }
            else if ((now - pressStartTick) >= pdMS_TO_TICKS(500))
            {
                /* Holding for 500+ ms — switch red LED on */
                ledRed.on();
                state = LONG_PRESS;
            }
            break;

        case LONG_PRESS:
            if (!pressed)
            {
                /* Long press released (>= 500 ms) — red stays on */
                TickType_t elapsed = now - pressStartTick;
                pressInfo.duration = (uint16_t)(elapsed * portTICK_PERIOD_MS);
                pressInfo.isLong   = true;
                Serial.println(F("[T1] Long press detected"));
                xSemaphoreGive(pressSemaphore);
                debounceStart = now;
                state = DEBOUNCE;
            }
            break;

        case DEBOUNCE:
            /* Ignore button for ~200 ms after release to prevent bounce */
            if ((now - debounceStart) >= pdMS_TO_TICKS(200))
            {
                state = IDLE;
            }
            break;
        }

        /* Delay 1 WDT tick ≈ 16 ms (WDT tick = 16 ms, so pdMS_TO_TICKS(10) rounds to 0!) */
        vTaskDelayUntil(&xLastWake, 1);
    }
}

/* ══════════════════════════════════════════════════════════
   Task 2 — Press Statistics & Yellow LED Blink
   Blocks on pressSemaphore (binary semaphore from Task 1).
   Updates shared PressStats under statsMutex protection.
   Blinks yellow LED: 5 times for short, 10 times for long.
   ══════════════════════════════════════════════════════════ */
void Tasks::pressStatsTask(void *pvParameters)
{
    (void)pvParameters;

    Serial.println(F("[T2] Press stats task started"));

    for (;;)
    {
        /* Block until Task 1 signals a completed press */
        if (xSemaphoreTake(pressSemaphore, portMAX_DELAY) == pdTRUE)
        {
            Serial.println(F("[T2] Semaphore received, processing"));
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

            /* Blink yellow LED: 5 blinks (short) or 10 blinks (long)
               Each blink = 50ms ON + 50ms OFF using vTaskDelay */
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
   Task 3 — Periodic Report (every 10 seconds via vTaskDelayUntil)
   Reads stats under statsMutex, prints report via Serial (STDIO),
   then resets all counters.
   ══════════════════════════════════════════════════════════ */
void Tasks::periodicReportTask(void *pvParameters)
{
    (void)pvParameters;

    Serial.println(F("[T3] Periodic report task started"));

    TickType_t xLastWake = xTaskGetTickCount();

    for (;;)
    {
        /* Wait exactly 10 seconds using vTaskDelayUntil */
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(10000));

        Serial.println(F("[T3] 10s elapsed, generating report"));

        /* Take a snapshot of stats under mutex, then reset */
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

        /* Print report via STDIO (Serial) */
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
