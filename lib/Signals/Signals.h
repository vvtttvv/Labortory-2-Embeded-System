/**
 * @file Signals.h
 * @brief Shared global signals for the provider / consumer model.
 *
 *  Signal                 │ Provider │ Consumer(s)
 *  -----------------------------------------------
 *  sig_pressDetected      │ Task 1   │ Task 2
 *  sig_pressDuration      │ Task 1   │ Task 2
 *  sig_isLongPress        │ Task 1   │ Task 2
 *  sig_totalPresses       │ Task 2   │ Task 3
 *  sig_shortPresses       │ Task 2   │ Task 3
 *  sig_longPresses        │ Task 2   │ Task 3
 *  sig_totalShortDuration │ Task 2   │ Task 3
 *  sig_totalLongDuration  │ Task 2   │ Task 3
 */
#ifndef SIGNALS_H
#define SIGNALS_H

#include <stdint.h>

extern volatile bool     sig_pressDetected;
extern volatile uint16_t sig_pressDuration;
extern volatile bool     sig_isLongPress;

extern volatile uint16_t sig_totalPresses;
extern volatile uint16_t sig_shortPresses;
extern volatile uint16_t sig_longPresses;
extern volatile uint32_t sig_totalShortDuration;
extern volatile uint32_t sig_totalLongDuration;

#endif
