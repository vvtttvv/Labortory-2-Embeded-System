#include "Signals.h"

volatile bool    sig_btnToggle     = false;
volatile bool    sig_btnDec        = false;
volatile bool    sig_btnInc        = false;
volatile bool    sig_led1State     = false;
volatile bool    sig_led2State     = false;
volatile int16_t sig_blinkInterval = 500;
