#include "hal/timer.h"
#include "hal/hal.h"

#include <stdint.h>

static uint64_t g_ticks;
static uint32_t g_tick_hz;

int hal_timer_init(uint32_t tick_hz) {
    if (tick_hz == 0U) {
        return -1;
    }

    g_ticks = 0U;
    g_tick_hz = tick_hz;
    return hal_timer_source_init(tick_hz);
}

int hal_timer_set_periodic(uint32_t tick_hz) {
    if (tick_hz == 0U) {
        return -1;
    }

    g_tick_hz = tick_hz;
    return hal_timer_source_init(tick_hz);
}

uint64_t hal_timer_monotonic_ticks(void) {
    return g_ticks;
}

void hal_timer_tick(void) {
    (void)g_tick_hz;
    g_ticks++;
}
