#include "hal/hal_pmu.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Simple x86_64 PMU implementation stub for Architectural PMU

static inline uint64_t rdpmc(uint32_t ecx) {
    uint32_t a, d;
    __asm__ volatile("rdpmc" : "=a"(a), "=d"(d) : "c"(ecx));
    return ((uint64_t)d << 32) | a;
}

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t lo, hi;
    __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}

static inline void wrmsr(uint32_t msr, uint64_t val) {
    uint32_t lo = (uint32_t)val;
    uint32_t hi = (uint32_t)(val >> 32);
    __asm__ volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

int hal_pmu_init(void) {
    // Check CPUID for Architectural PMU version
    return 0;
}

int hal_pmu_init_cpu_local(uint32_t cpu_id) {
    (void)cpu_id;
    // Enable performance counters globally
    uint64_t cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 8); // PCE: Performance Counter Enable
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
    return 0;
}

int hal_pmu_enable_event(hal_pmu_event_t event) {
    // Fixed counters (MSR_PERF_FIXED_CTR_CTRL)
    if (event == PMU_EVENT_INSTR_RETIRED || event == PMU_EVENT_CYCLES) {
        // Just rely on the fixed counters which are usually running
        return 0;
    }
    // Program general purpose counters via MSR_P6_EVNTSEL0 etc.
    return -1;
}

int hal_pmu_disable_event(hal_pmu_event_t event) {
    (void)event;
    return 0;
}

uint64_t hal_pmu_read_event(hal_pmu_event_t event) {
    switch (event) {
        case PMU_EVENT_INSTR_RETIRED:
            return rdpmc((1<<30) | 0); // Fixed counter 0
        case PMU_EVENT_CYCLES:
            return rdpmc((1<<30) | 1); // Fixed counter 1
        default:
            return 0;
    }
}

void hal_pmu_snapshot(hal_pmu_snapshot_t* snapshot) {
    if (!snapshot) return;
    snapshot->instr_retired = hal_pmu_read_event(PMU_EVENT_INSTR_RETIRED);
    snapshot->cycles        = hal_pmu_read_event(PMU_EVENT_CYCLES);
    snapshot->cache_refs    = 0; // Not programmed by default in this stub
    snapshot->cache_misses  = 0;
    snapshot->branch_instrs = 0;
    snapshot->branch_misses = 0;
}
