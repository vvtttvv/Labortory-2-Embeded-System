#ifndef SIGNALS_H
#define SIGNALS_H

#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <stdint.h>

struct PressInfo {
    uint16_t duration;
    bool     isLong;
};

// Aggregated statistics: Task2 Task3 (protected by statsMutex)
struct PressStats {
    uint16_t totalPresses;
    uint16_t shortPresses;
    uint16_t longPresses;
    uint32_t totalShortDuration;
    uint32_t totalLongDuration;
};

// Global FreeRTOS handles
extern SemaphoreHandle_t pressSemaphore;   // binary semaphore
extern SemaphoreHandle_t statsMutex;       // mutex

// Shared data
extern volatile PressInfo pressInfo;
extern PressStats         stats;

// Initialization
void Signals_init(void);

// Validation (hides FreeRTOS handles from callers)
bool Signals_isReady(void);

#endif
