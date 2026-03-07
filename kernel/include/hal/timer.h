#ifndef BHARAT_HAL_TIMER_H
#define BHARAT_HAL_TIMER_H

#include <stdint.h>

int hal_timer_init(uint32_t tick_hz);
int hal_timer_set_periodic(uint32_t tick_hz);
uint64_t hal_timer_monotonic_ticks(void);
void hal_timer_tick(void);

#endif // BHARAT_HAL_TIMER_H
