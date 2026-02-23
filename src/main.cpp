#include <Arduino.h>
#include <stdio.h>
#include "UartStdio.h"
#include "Signals.h"
#include "Tasks.h"
#include "SerialCmd.h"
#include "Scheduler.h"

#define TASK2_REC_MS 50

void setup()
{
    UartStdio::init(9600);
    Tasks::initHardware();

    Scheduler::init();
    Scheduler::addTask(Tasks::buttonScan,    50, 0);
    Scheduler::addTask(Tasks::buttonLed,     50, 5);
    Scheduler::addTask(Tasks::blinkLed,      TASK2_REC_MS, 15);
    Scheduler::addTask(Tasks::stateVariable, 50, 25);

    printf("System ready\n");
    printf("T0: BtnScan   rec=50ms  off=0ms\n");
    printf("T1: ButtonLED rec=50ms  off=5ms\n");
    printf("T2: BlinkLED  rec=50ms  off=15ms\n");
    printf("T3: StateVar  rec=50ms  off=25ms\n");
    printf("Idle: Report  (main loop, 1s)\n");
    printf("Btn: D8=toggle D7=dec D2=inc\n");
}

void loop()
{
    Scheduler::dispatch();
    SerialCmd::idleReport();
    SerialCmd::poll();
}