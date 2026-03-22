#include "hal/hal_timer.h"
#include "hal/hal.h"
#include "sched.h"

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
