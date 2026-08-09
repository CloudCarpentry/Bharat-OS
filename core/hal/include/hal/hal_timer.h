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

// Absolute Monotonic Time
bool hal_timer_monotonic_ns(uint64_t *out_ns);
bool hal_timer_program_deadline_ns(uint64_t absolute_deadline_ns);

// Legacy compatibility
uint64_t hal_timer_read_ns(void);
void hal_timer_program_oneshot_ns(uint64_t ns_from_now);

// Trigger timer tick processing
void hal_timer_tick(void);

bool hal_timer_is_per_cpu(void);

// Timer Capabilities
typedef struct {
    bool has_counter;
    bool has_monotonic_ns;
    bool has_precise_oneshot;
    bool has_native_absolute_deadline;
    bool is_per_cpu;
} hal_timer_caps_t;

const hal_timer_caps_t* hal_timer_get_capabilities(void);

// Common helper for overflow-safe ceiling conversion
bool hal_timer_ns_to_ticks_ceil(uint64_t ns, uint64_t hz, uint64_t *out_ticks);

#endif // BHARAT_HAL_TIMER_H
