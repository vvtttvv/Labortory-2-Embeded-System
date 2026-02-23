/**
 * @file Signals.h
 * @brief Shared global signals for the provider / consumer model.
 *
 *  Signal            │ Provider │ Consumer(s)
 *  -----------------------------------------------
 *  sig_btnToggle     │ Task 0   │ Task 1
 *  sig_btnDec        │ Task 0   │ Task 3
 *  sig_btnInc        │ Task 0   │ Task 3
 *  sig_led1State     │ Task 1   │ Task 2, Idle
 *  sig_led2State     │ Task 2   │ Idle
 *  sig_blinkInterval │ Task 3   │ Task 2, Idle
 */
#ifndef SIGNALS_H
#define SIGNALS_H

#include <stdint.h>

extern volatile bool    sig_btnToggle;     // Button 1 pressed (toggle LED1)
extern volatile bool    sig_btnDec;        // Button 2 pressed (decrement interval)
extern volatile bool    sig_btnInc;        // Button 3 pressed (increment interval)
extern volatile bool    sig_led1State;     // LED1 state (green)
extern volatile bool    sig_led2State;     // LED2 state (yellow, blink)
extern volatile int16_t sig_blinkInterval; // Blink period in ms

#endif
