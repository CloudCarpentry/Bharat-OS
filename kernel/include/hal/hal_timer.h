#ifndef BHARAT_HAL_TIMER_H
#define BHARAT_HAL_TIMER_H

#include <stdint.h>

// Initialize timer source on boot core
int hal_timer_init(uint32_t tick_hz);

// Initialize per-core timer event source
int hal_timer_init_cpu_local(uint32_t tick_hz);

// Set mode
int hal_timer_set_periodic(uint32_t tick_hz);
int hal_timer_set_oneshot(uint64_t ns_delay);

// Read time
uint64_t hal_timer_monotonic_ticks(void);

// Trigger timer tick processing
void hal_timer_tick(void);

#endif
