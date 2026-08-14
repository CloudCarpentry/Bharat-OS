#include "hal/hal_timer.h"

/* UP-only fallback clock: the boot core is the sole reader/writer. */
static uint64_t g_arm32_timer_ticks;

void hal_timer_init(void) {
    g_arm32_timer_ticks = 0;
}

void hal_timer_init_cpu_local(uint32_t cpu_id) {
    (void)cpu_id;
}

void hal_timer_program_periodic(uint64_t ns) {
    (void)ns;
}

void hal_timer_program_oneshot(uint64_t ns) {
    (void)ns;
}

uint64_t hal_timer_read_counter(void) {
    return ++g_arm32_timer_ticks;
}

uint64_t hal_timer_read_freq(void) {
    return 1000000ULL;
}

uint64_t hal_timer_monotonic_ticks_arch(void) {
    return hal_timer_read_counter();
}

bool hal_timer_is_per_cpu(void) {
    return false;
}

void hal_timer_arch_get_caps(hal_timer_caps_t *caps) {
    caps->has_counter = true;
    caps->has_monotonic_ns = false; // Degraded: Uses software counter increment. Follow-up: Platform-Discovered ARM32 Timebase.
    caps->has_precise_oneshot = false; // Degraded: Untested precision.
    caps->has_native_absolute_deadline = false;
    caps->is_per_cpu = false;
}
