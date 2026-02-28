#include "Signals.h"

/* ── FreeRTOS handles ── */
SemaphoreHandle_t pressSemaphore = NULL;
SemaphoreHandle_t statsMutex     = NULL;

/* ── Shared data ── */
volatile PressInfo pressInfo = { 0, false };
PressStats         stats     = { 0, 0, 0, 0, 0 };

void Signals_init(void)
{
    pressSemaphore = xSemaphoreCreateBinary();
    statsMutex     = xSemaphoreCreateMutex();
}
