#include "hal/hal_pmu.h"
#include "hal/hal_discovery.h"
#include <stddef.h>

// Simple ARMv8 PMUv3 abstraction stub

static inline uint64_t pmccntr_read(void) {
    uint64_t val;
    __asm__ volatile("mrs %0, PMCCNTR_EL0" : "=r"(val));
    return val;
}

static inline void pmccntr_write(uint64_t val) {
    __asm__ volatile("msr PMCCNTR_EL0, %0" : : "r"(val));
}

int hal_pmu_init(void) {
    // Check common discovery for PMU_ARMV8
    system_discovery_t* disc = hal_get_system_discovery();
    if (!disc) return -1;

    bool pmu_found = false;
    for (uint32_t i = 0; i < disc->pmu_count; i++) {
        if (disc->pmus[i].type == PMU_ARMV8) {
            pmu_found = true;
            break;
        }
    }

    if (!pmu_found) return -1;

    // Global initialisation
    return 0;
}

int hal_pmu_init_cpu_local(uint32_t cpu_id) {
    (void)cpu_id;
    // Enable PMU across all modes
    // Set PMCR_EL0.E
    uint32_t pmcr;
    __asm__ volatile("mrs %0, PMCR_EL0" : "=r"(pmcr));
    pmcr |= 1; // E: Enable PMU
    __asm__ volatile("msr PMCR_EL0, %0" : : "r"(pmcr));
    __asm__ volatile("isb");

    // Enable cycle counter PMCNTENSET_EL0
    __asm__ volatile("msr PMCNTENSET_EL0, %0" : : "r"(1ULL << 31));
    return 0;
}

int hal_pmu_enable_event(hal_pmu_event_t event) {
    if (event == PMU_EVENT_CYCLES || event == PMU_EVENT_INSTR_RETIRED) {
        // Assume enabled for stub
        return 0;
    }
    return -1; // Unimplemented
}

int hal_pmu_disable_event(hal_pmu_event_t event) {
    (void)event;
    return 0;
}

uint64_t hal_pmu_read_event(hal_pmu_event_t event) {
    if (event == PMU_EVENT_CYCLES) {
        return pmccntr_read();
    } else if (event == PMU_EVENT_INSTR_RETIRED) {
        // Need to program EVTYPER for Instruction retired and read PMEVCNTR
        // For stub, return cycle as approximation if real event not mapped
        return 0;
    }
    return 0;
}

void hal_pmu_snapshot(hal_pmu_snapshot_t* snapshot) {
    if (!snapshot) return;
    snapshot->cycles = hal_pmu_read_event(PMU_EVENT_CYCLES);
    snapshot->instr_retired = hal_pmu_read_event(PMU_EVENT_INSTR_RETIRED);
    // Other counters not programmed by default in this stub
    snapshot->cache_refs    = 0;
    snapshot->cache_misses  = 0;
    snapshot->branch_instrs = 0;
    snapshot->branch_misses = 0;
}
