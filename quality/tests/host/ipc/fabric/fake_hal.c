#include "fake_hal.h"
#include <hal/hal_ipi.h>
#include "ipc/mk_proto.h"

static _Thread_local uint32_t g_fake_cpu_id = 0;
static uint64_t g_fake_ticks = 0;
static uint32_t g_fake_ipi_counters[256];

void fake_hal_set_cpu_id(uint32_t cpu_id) {
    g_fake_cpu_id = cpu_id;
}

uint32_t hal_cpu_get_id(void) {
    return g_fake_cpu_id;
}

uint64_t hal_timer_monotonic_ticks(void) {
    return g_fake_ticks;
}

void fake_hal_set_ticks(uint64_t ticks) {
    g_fake_ticks = ticks;
}

void fake_hal_ticks_add(uint64_t delta) {
    g_fake_ticks += delta;
}

void hal_ipi_send(uint32_t target_cpu, hal_ipi_reason_t reason) {
    if (target_cpu < 256) {
        g_fake_ipi_counters[target_cpu]++;
    }
}

uint32_t fake_hal_get_ipi_count(uint32_t target_cpu) {
    if (target_cpu < 256) {
        return g_fake_ipi_counters[target_cpu];
    }
    return 0;
}

void fake_hal_reset_ipi_counters(void) {
    for (int i = 0; i < 256; i++) {
        g_fake_ipi_counters[i] = 0;
    }
}

int mk_dispatch_legacy_adapter(const bh_mk_wire_message_t *msg) {
    (void)msg;
    return K_OK;
}
