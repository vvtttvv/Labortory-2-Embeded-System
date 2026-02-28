#ifndef TASKS_H
#define TASKS_H

#include <Arduino_FreeRTOS.h>

namespace Tasks
{
    void initHardware();

    /* FreeRTOS task functions (infinite-loop, never return) */
    void buttonMonitorTask(void *pvParameters);  // Task 1
    void pressStatsTask(void *pvParameters);      // Task 2
    void periodicReportTask(void *pvParameters);  // Task 3
}

#endif
