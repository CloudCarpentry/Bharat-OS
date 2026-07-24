#ifndef BHARAT_HAL_PMU_H
#define BHARAT_HAL_PMU_H

#include <stdint.h>
#include <stdbool.h>

// --- PMU Event Types ---
typedef enum {
    PMU_EVENT_CYCLES,              // Total CPU cycles
    PMU_EVENT_INSTR_RETIRED,       // Instructions retired
    PMU_EVENT_CACHE_REFERENCES,    // L1/L2 cache references (arch specific)
    PMU_EVENT_CACHE_MISSES,        // L1/L2 cache misses
    PMU_EVENT_BRANCH_INSTRUCTIONS, // Branch instructions executed
    PMU_EVENT_BRANCH_MISSES        // Branch mispredictions
} hal_pmu_event_t;

// Event bit-mask helpers for telemetry consumers.
#define PMU_EVENT_MASK_CYCLES               (1u << PMU_EVENT_CYCLES)
#define PMU_EVENT_MASK_INSTR_RETIRED        (1u << PMU_EVENT_INSTR_RETIRED)
#define PMU_EVENT_MASK_CACHE_REFERENCES     (1u << PMU_EVENT_CACHE_REFERENCES)
#define PMU_EVENT_MASK_CACHE_MISSES         (1u << PMU_EVENT_CACHE_MISSES)
#define PMU_EVENT_MASK_BRANCH_INSTRUCTIONS  (1u << PMU_EVENT_BRANCH_INSTRUCTIONS)
#define PMU_EVENT_MASK_BRANCH_MISSES        (1u << PMU_EVENT_BRANCH_MISSES)

// --- PMU State ---
typedef struct {
    uint64_t cycles;
    uint64_t instr_retired;
    uint64_t cache_refs;
    uint64_t cache_misses;
    uint64_t branch_instrs;
    uint64_t branch_misses;

    // 1 if one or more counters are heuristics/fallback values rather than
    // hardware-programmed architectural events.
    uint8_t approximate;

    // Bit mask of PMU_EVENT_MASK_* entries that are backed by real hardware
    // counters on this platform.
    uint32_t supported_events_mask;
} hal_pmu_snapshot_t;

// Init the PMU subsystem (called early boot)
int hal_pmu_init(void);

// Init PMU for the local CPU core
int hal_pmu_init_cpu_local(uint32_t cpu_id);

// Start/Stop counting a specific event
int hal_pmu_enable_event(hal_pmu_event_t event);
int hal_pmu_disable_event(hal_pmu_event_t event);

// Read current count for a specific event
uint64_t hal_pmu_read_event(hal_pmu_event_t event);

// Snapshot all enabled basic events for telemetry (scheduler)
void hal_pmu_snapshot(hal_pmu_snapshot_t* snapshot);

#endif // BHARAT_HAL_PMU_H
