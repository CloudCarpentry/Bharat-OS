#ifndef BHARAT_HAL_TIMER_H
#define BHARAT_HAL_TIMER_H

#include <stdint.h>
#include <stdbool.h>

// Initialize timer source on boot core
void hal_timer_init(void);

// Initialize per-core timer event source
void hal_timer_init_cpu_local(uint32_t cpu_id);

// Set mode
void hal_timer_program_periodic(uint64_t ns);
void hal_timer_program_oneshot(uint64_t ns);

// Read time
uint64_t hal_timer_read_counter(void);
uint64_t hal_timer_read_freq(void);
uint64_t hal_timer_monotonic_ticks(void);

// Trigger timer tick processing
void hal_timer_tick(void);

bool hal_timer_is_per_cpu(void);

#endif // BHARAT_HAL_TIMER_H
