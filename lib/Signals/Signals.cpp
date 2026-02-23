#include "Signals.h"

volatile char    sig_key           = '\0';
volatile bool    sig_led1State     = false;
volatile bool    sig_led2State     = false;
volatile int16_t sig_blinkInterval = 500;
