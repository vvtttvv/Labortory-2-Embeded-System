/**
 * @file Signals.h
 * @brief FreeRTOS synchronization primitives and shared data for inter-task communication.
 *
 *  Mechanism              │ Purpose
 *  ------------------------------------------------
 *  pressSemaphore         │ Binary semaphore: Task1 gives on press detected, Task2 takes
 *  statsMutex             │ Mutex: protects shared statistics (Task2 writes, Task3 reads/resets)
 *  pressInfo              │ Struct with press duration & type, written by Task1 before giving semaphore
 *  stats                  │ Struct with aggregated statistics, protected by statsMutex
 */
#ifndef SIGNALS_H
#define SIGNALS_H

#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <stdint.h>

/* ── Press info: Task1 → Task2 (protected by semaphore handoff) ── */
struct PressInfo {
    uint16_t duration;    // press duration in ms
    bool     isLong;      // true if >= 500 ms
};

/* ── Aggregated statistics: Task2 ↔ Task3 (protected by statsMutex) ── */
struct PressStats {
    uint16_t totalPresses;
    uint16_t shortPresses;
    uint16_t longPresses;
    uint32_t totalShortDuration;
    uint32_t totalLongDuration;
};

/* ── Global FreeRTOS handles ── */
extern SemaphoreHandle_t pressSemaphore;   // binary semaphore
extern SemaphoreHandle_t statsMutex;       // mutex

/* ── Shared data ── */
extern volatile PressInfo pressInfo;
extern PressStats         stats;

/* ── Initialization ── */
void Signals_init(void);

#endif
