#ifndef BHARAT_HAL_OLD_TIMER_H
#define BHARAT_HAL_OLD_TIMER_H

#include "hal_timer.h"

// Transitional wrapper for old hal.h
int hal_timer_source_init(uint32_t tick_hz);

#endif // BHARAT_HAL_OLD_TIMER_H
