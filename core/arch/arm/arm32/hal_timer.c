#include "hal/hal_timer.h"
#include "hal/hal_discovery.h"

// Currently no generic platform-independent ARM32 timer is implemented.
// We defer to system discovery if an SP804 or similar is found later.
static uint32_t g_arm32_timer_freq = 0;

void hal_timer_init(void) {
    system_discovery_t* discovery = hal_get_system_discovery();
    if (discovery && discovery->timers[0].frequency > 0) {
        g_arm32_timer_freq = discovery->timers[0].frequency;
    }
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
    // Cannot return a software loop counter. If we don't have hardware, return 0.
    return 0;
}

uint64_t hal_timer_read_freq(void) {
    return g_arm32_timer_freq;
}

uint64_t hal_timer_monotonic_ticks_arch(void) {
    return hal_timer_read_counter();
}

bool hal_timer_is_per_cpu(void) {
    return false;
}

void hal_timer_arch_get_caps(hal_timer_caps_t *caps) {
    caps->has_counter = (g_arm32_timer_freq > 0);
    caps->has_monotonic_ns = (g_arm32_timer_freq > 0);
    caps->has_precise_oneshot = false;
    caps->has_native_absolute_deadline = false;
    caps->is_per_cpu = false;
}
