#include "hal/hal_pmu.h"
#include <stddef.h>
#include <stdint.h>

// Conservative x86_64 PMU implementation.
//
// NOTE: We intentionally use RDTSC for cycles instead of RDPMC fixed counters
// because some virtualized environments (including common QEMU configs) expose
// Architectural PMU metadata but still raise #GP for RDPMC access.

static inline uint64_t rdtsc_ordered(void) {
    uint32_t lo, hi;
    __asm__ volatile("lfence\n\trdtsc" : "=a"(lo), "=d"(hi) : : "memory");
    return ((uint64_t)hi << 32) | lo;
}

int hal_pmu_init(void) {
    // TODO: Check CPUID for Architectural PMU version and capabilities.
    return 0;
}

int hal_pmu_init_cpu_local(uint32_t cpu_id) {
    (void)cpu_id;
    return 0;
}

int hal_pmu_enable_event(hal_pmu_event_t event) {
    if (event == PMU_EVENT_CYCLES || event == PMU_EVENT_INSTR_RETIRED) {
        return 0;
    }

    return -1;
}

int hal_pmu_disable_event(hal_pmu_event_t event) {
    (void)event;
    return 0;
}

uint64_t hal_pmu_read_event(hal_pmu_event_t event) {
    if (event == PMU_EVENT_CYCLES) {
        return rdtsc_ordered();
    }

    // We do not expose a fake instruction counter. Callers should use
    // snapshot->approximate/supported_events_mask when consuming telemetry.
    return 0;
}

void hal_pmu_snapshot(hal_pmu_snapshot_t* snapshot) {
    if (!snapshot) return;

    snapshot->cycles = hal_pmu_read_event(PMU_EVENT_CYCLES);
    snapshot->instr_retired = hal_pmu_read_event(PMU_EVENT_INSTR_RETIRED);
    snapshot->cache_refs = 0;
    snapshot->cache_misses = 0;
    snapshot->branch_instrs = 0;
    snapshot->branch_misses = 0;

    snapshot->approximate = 1u;
    snapshot->supported_events_mask = PMU_EVENT_MASK_CYCLES;
}
