#include <Arduino.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include "UartStdio.h"
#include "Signals.h"
#include "Tasks.h"

void setup()
{
    UartStdio::init(9600);

    // I used PSTR because it keeps the string in flash memory, saving precious RAM on the Arduino
    printf_P(PSTR("[INIT] FreeRTOS Button Monitor\n"));

    Tasks::initHardware();
    Signals_init();

    if (!Signals_isReady())
    {
        printf_P(PSTR("[ERR] Sync objects failed\n"));
        for (;;) { ; }
    }

    printf_P(PSTR("[INIT] Creating tasks...\n"));

    if (!Tasks::createAll())
    {
        printf_P(PSTR("[ERR] Task creation failed\n"));
        for (;;) { ; }
    }

    printf_P(PSTR("[INIT] Tasks created, scheduler will start after setup returns\n"));
}

void loop(){}
