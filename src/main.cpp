#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include "Signals.h"
#include "Tasks.h"

void setup()
{
    Serial.begin(9600);

    for (volatile uint16_t i = 0; i < 30000; i++) { ; }

    Serial.println(F("[INIT] FreeRTOS Button Monitor"));

    Tasks::initHardware();
    Signals_init();

    if (pressSemaphore == NULL || statsMutex == NULL)
    {
        Serial.println(F("[ERR] Sync objects failed"));
        for (;;) { ; }
    }

    BaseType_t r1, r2, r3;

    Serial.println(F("[INIT] Creating tasks..."));
    
    r1 = xTaskCreate(Tasks::buttonMonitorTask,
                     "BtnMon",
                     256,
                     NULL,
                     2,
                     NULL);

    r2 = xTaskCreate(Tasks::pressStatsTask,
                     "Stats",
                     256,
                     NULL,
                     2,
                     NULL);

    r3 = xTaskCreate(Tasks::periodicReportTask,
                     "Report",
                     384,
                     NULL,
                     1,
                     NULL);

    if (r1 != pdPASS || r2 != pdPASS || r3 != pdPASS)
    {
        Serial.print(F("[ERR] xTaskCreate: "));
        Serial.print(r1); Serial.print(' ');
        Serial.print(r2); Serial.print(' ');
        Serial.println(r3);
        for (;;) { ; }
    }

    Serial.println(F("[INIT] Tasks created, scheduler will start after setup returns"));
}

void loop(){}
