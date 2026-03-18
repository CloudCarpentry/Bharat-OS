#include "hal/hal_pmu.h"
#include "hal/hal_discovery.h"
#include "../../boot/riscv/sbi.h"
#include <stddef.h>

// SBI PMU Extension implementation
// (Stubbed logic mapping common telemetry queries to SBI hardware performance counters)

int hal_pmu_init(void) {
    // Rely on system_discovery_t from FDT to confirm SBI PMU presence
    system_discovery_t* disc = hal_get_system_discovery();
    if (!disc) return -1;

    bool pmu_found = false;
    for (uint32_t i = 0; i < disc->pmu_count; i++) {
        if (disc->pmus[i].type == PMU_RISCV_SBI) {
            pmu_found = true;
            break;
        }
    }

    if (!pmu_found) return -1;

    // Probe SBI extension
    return 0; // Returning 0 on success
}

int hal_pmu_init_cpu_local(uint32_t cpu_id) {
    (void)cpu_id;
    // Enable performance counters in MIE / SIE if delegating interrupts
    return 0;
}

int hal_pmu_enable_event(hal_pmu_event_t event) {
    (void)event;
    // For standard cycle/instret, we just read CSRs
    return 0; // Built-in counters are always running
}

int hal_pmu_disable_event(hal_pmu_event_t event) {
    (void)event;
    return 0;
}

uint64_t hal_pmu_read_event(hal_pmu_event_t event) {
    uint64_t val = 0;
    if (event == PMU_EVENT_CYCLES) {
        __asm__ volatile("rdcycle %0" : "=r"(val)); // Read unprivileged cycle CSR
    } else if (event == PMU_EVENT_INSTR_RETIRED) {
        __asm__ volatile("rdinstret %0" : "=r"(val)); // Read unprivileged instret CSR
    }
    return val;
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
