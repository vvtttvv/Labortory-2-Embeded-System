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

// Task 1 -> Task 2: button press event
extern volatile bool     sig_pressDetected;      // new press detected
extern volatile uint16_t sig_pressDuration;      // duration in ms
extern volatile bool     sig_isLongPress;        // true if >= 500 ms

// Task 2 -> Task 3: statistics (reset by Task 3)
extern volatile uint16_t sig_totalPresses;
extern volatile uint16_t sig_shortPresses;
extern volatile uint16_t sig_longPresses;
extern volatile uint32_t sig_totalShortDuration; // sum of short durations
extern volatile uint32_t sig_totalLongDuration;  // sum of long durations

#endif
