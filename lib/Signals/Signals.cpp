#include "Signals.h"

SemaphoreHandle_t pressSemaphore = NULL;
SemaphoreHandle_t statsMutex     = NULL;

volatile PressInfo pressInfo = { 0, false };
PressStats         stats     = { 0, 0, 0, 0, 0 };

void Signals_init(void)
{
    pressSemaphore = xSemaphoreCreateBinary();
    statsMutex     = xSemaphoreCreateMutex();
}

bool Signals_isReady(void)
{
    return (pressSemaphore != NULL && statsMutex != NULL);
}
