#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include "Signals.h"
#include "Tasks.h"

void setup()
{
    Serial.begin(9600);

    Tasks::initHardware();
    Signals_init();

    Serial.println(F("=== FreeRTOS Button Monitor ==="));
    Serial.println(F("Task 1: ButtonMonitor  (poll 10ms)"));
    Serial.println(F("Task 2: PressStats     (semaphore-driven)"));
    Serial.println(F("Task 3: PeriodicReport (every 10s)"));

    /* Create FreeRTOS tasks
       Stack sizes in bytes (AVR StackType_t = uint8_t).
       Priority: Task1 highest (time-critical), Task3 lowest. */
    BaseType_t r1, r2, r3;

    r1 = xTaskCreate(Tasks::buttonMonitorTask,
                     "BtnMon",
                     196,
                     NULL,
                     3,
                     NULL);

    r2 = xTaskCreate(Tasks::pressStatsTask,
                     "Stats",
                     196,
                     NULL,
                     2,
                     NULL);

    r3 = xTaskCreate(Tasks::periodicReportTask,
                     "Report",
                     256,
                     NULL,
                     1,
                     NULL);

    if (r1 != pdPASS || r2 != pdPASS || r3 != pdPASS)
    {
        Serial.println(F("ERROR: xTaskCreate failed!"));
        for (;;) { ; }
    }

    Serial.println(F("All tasks created. Starting scheduler..."));

    /* Explicitly start the FreeRTOS scheduler */
    vTaskStartScheduler();

    /* Should never reach here */
}

void loop()
{
    /* Never reached — FreeRTOS scheduler takes over */
}
