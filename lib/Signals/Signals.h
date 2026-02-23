/**
 * @file Signals.h
 * @brief Shared global signals for the provider / consumer model.
 *
 *  Signal            │ Provider │ Consumer(s)
 *  ---------------------------------------------
 *  sig_key           │ Task 0   │ Task 1, Task 3
 *  sig_led1State     │ Task 1   │ Task 2, Idle
 *  sig_led2State     │ Task 2   │ Idle
 *  sig_blinkInterval │ Task 3   │ Task 2, Idle
 */
#ifndef SIGNALS_H
#define SIGNALS_H

#include <stdint.h>

extern volatile char sig_key;
extern volatile bool sig_led1State;
extern volatile bool sig_led2State;
extern volatile int16_t sig_blinkInterval;

#endif
