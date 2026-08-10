#include "hal/hal_timer.h"
#include "hal/hal.h"
#include "sched/sched.h"

#include <stdint.h>

static uint64_t g_ticks;
static uint32_t g_tick_hz;

void hal_timer_tick(void) {
    (void)g_tick_hz;
    g_ticks++;
    sched_on_timer_tick();
}

uint64_t hal_timer_monotonic_ticks(void) {
    return g_ticks;
}

// Architecture-provided hook for capabilities truth
extern void hal_timer_arch_get_caps(hal_timer_caps_t *caps) __attribute__((weak));

static hal_timer_caps_t g_timer_caps;
static bool g_timer_caps_init = false;

const hal_timer_caps_t* hal_timer_get_capabilities(void) {
    if (!g_timer_caps_init) {
        g_timer_caps.has_counter = false;
        g_timer_caps.has_monotonic_ns = false;
        g_timer_caps.has_precise_oneshot = false;
        g_timer_caps.has_native_absolute_deadline = false;
        g_timer_caps.is_per_cpu = hal_timer_is_per_cpu();

        if (hal_timer_arch_get_caps) {
            hal_timer_arch_get_caps(&g_timer_caps);
        } else {
            // Degraded default if backend does not explicitly claim otherwise.
            uint64_t freq = hal_timer_read_freq();
            if (freq > 0) {
                g_timer_caps.has_counter = true;
            }
        }
        g_timer_caps_init = true;
    }
    return &g_timer_caps;
}

bool hal_timer_ns_to_ticks_ceil(uint64_t ns, uint64_t hz, uint64_t *out_ticks) {
    if (hz == 0 || out_ticks == NULL) {
        return false;
    }
    if (ns == 0) {
        *out_ticks = 0;
        return true;
    }

    uint64_t sec = ns / 1000000000ULL;
    uint64_t frac_ns = ns % 1000000000ULL;
    uint64_t ticks = (sec * hz) + ((frac_ns * hz + 999999999ULL) / 1000000000ULL);

    if (ticks == 0) {
        ticks = 1;
    }
    *out_ticks = ticks;
    return true;
}

bool hal_timer_monotonic_ns(uint64_t *out_ns) {
    if (!out_ns) return false;

    const hal_timer_caps_t *caps = hal_timer_get_capabilities();
    if (!caps->has_monotonic_ns) {
        return false;
    }

    uint64_t freq = hal_timer_read_freq();
    if (freq == 0) {
        return false;
    }

    uint64_t counter = hal_timer_read_counter();

    uint64_t sec = counter / freq;
    uint64_t rem = counter % freq;
    *out_ns = (sec * 1000000000ULL) + ((rem * 1000000000ULL) / freq);

    return true;
}

bool hal_timer_program_deadline_ns(uint64_t absolute_deadline_ns) {
    uint64_t now;
    if (!hal_timer_monotonic_ns(&now)) {
        return false;
    }
    if (now >= absolute_deadline_ns) {
        return false;
    }

    uint64_t remaining_ns = absolute_deadline_ns - now;
    hal_timer_program_oneshot(remaining_ns);
    return true;
}

// These provide compatibility for the extra header in core/hal/include/hal/hal_timer.h
uint64_t hal_timer_read_ns(void) {
    uint64_t ns = 0;
    hal_timer_monotonic_ns(&ns);
    return ns;
}

void hal_timer_program_oneshot_ns(uint64_t ns_from_now) {
    hal_timer_program_oneshot(ns_from_now);
}
