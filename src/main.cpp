#include <Arduino.h>
#include <stdio.h>
#include "UartStdio.h"
#include "Signals.h"
#include "Tasks.h"
#include "Scheduler.h"

void setup()
{
    UartStdio::init(9600);
    Tasks::initHardware();

    scheduler_init();
    scheduler_addTask(Tasks::buttonMonitor,   10,    0);
    scheduler_addTask(Tasks::pressStats,      50,    5);
    scheduler_addTask(Tasks::periodicReport, 10000, 15);

    printf("Menu:\n");
    printf("T1: ButtonMonitor  rec=10ms     off=0ms\n");
    printf("T2: PressStats     rec=50ms     off=5ms\n");
    printf("T3: PeriodicReport rec=10000ms  off=15ms\n");
}

void loop()
{
    scheduler_loop();
}