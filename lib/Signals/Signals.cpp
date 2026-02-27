#include "Signals.h"

// Task 1 -> Task 2
volatile bool     sig_pressDetected      = false;
volatile uint16_t sig_pressDuration      = 0;
volatile bool     sig_isLongPress        = false;

// Task 2 -> Task 3
volatile uint16_t sig_totalPresses       = 0;
volatile uint16_t sig_shortPresses       = 0;
volatile uint16_t sig_longPresses        = 0;
volatile uint32_t sig_totalShortDuration = 0;
volatile uint32_t sig_totalLongDuration  = 0;
