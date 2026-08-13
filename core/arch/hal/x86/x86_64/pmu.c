#include "hal/hal_pmu.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define CPUID_LEAF_PMU 0xA
#define MSR_IA32_PERFEVTSEL0 0x186
#define MSR_IA32_PMC0 0xC1
#define MSR_IA32_PERF_GLOBAL_CTRL 0x38F

static uint8_t pmu_version = 0;
static uint8_t num_general_counters = 0;
static uint32_t unsupported_events_mask = 0;
static uint32_t supported_mask = 0;

static inline void x86_cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    __asm__ volatile("cpuid" : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) : "a"(leaf));
}

static inline void x86_wrmsr(uint32_t msr, uint64_t value) {
    uint32_t low = (uint32_t)value;
    uint32_t high = (uint32_t)(value >> 32);
    __asm__ volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

static inline uint64_t x86_rdmsr(uint32_t msr) {
    uint32_t low, high;
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

static inline uint64_t rdtsc_ordered(void) {
    uint32_t lo, hi;
    __asm__ volatile("lfence\n\trdtsc" : "=a"(lo), "=d"(hi) : : "memory");
    return ((uint64_t)hi << 32) | lo;
}

// Map from hal_pmu_event_t to actual PMC index, or -1 if unmapped/unsupported
static int event_to_pmc[6] = {-1, -1, -1, -1, -1, -1};
// Keep track of which PMCs are used
static bool pmc_in_use[8] = {false};

// Event select values for standard architectural events
// Format: Event Select (Bits 0-7), Umask (Bits 8-15), USR (Bit 16), OS (Bit 17), EN (Bit 22)
#define PMU_EVT_EN (1ULL << 22)
#define PMU_EVT_OS (1ULL << 17)
#define PMU_EVT_USR (1ULL << 16)
#define PMU_BASE_CFG (PMU_EVT_EN | PMU_EVT_OS | PMU_EVT_USR)

static const uint64_t arch_events[] = {
    0x003C | PMU_BASE_CFG, // PMU_EVENT_CYCLES (Unhalted Core Cycles, Event 3C, Umask 00)
    0x00C0 | PMU_BASE_CFG, // PMU_EVENT_INSTR_RETIRED (Instruction Retired, Event C0, Umask 00)
    0x4F2E | PMU_BASE_CFG, // PMU_EVENT_CACHE_REFERENCES (LLC Reference, Event 2E, Umask 4F)
    0x412E | PMU_BASE_CFG, // PMU_EVENT_CACHE_MISSES (LLC Misses, Event 2E, Umask 41)
    0x00C4 | PMU_BASE_CFG, // PMU_EVENT_BRANCH_INSTRUCTIONS (Branch Instruction Retired, Event C4, Umask 00)
    0x00C5 | PMU_BASE_CFG  // PMU_EVENT_BRANCH_MISSES (Branch Misses Retired, Event C5, Umask 00)
};

static const uint32_t arch_event_bit_idx[] = {
    0, // Unhalted core cycles
    1, // Instruction retired
    2, // Reference cycles (not exactly cache refs, but close in arch events)
    3, // LLC Reference
    4, // LLC Misses
    5, // Branch Instruction Retired
    6  // Branch Misses Retired
};

// Mapping our events to intel arch event bit index (for unsupported mask check)
static const int our_event_to_intel_bit[] = {
    0, // PMU_EVENT_CYCLES
    1, // PMU_EVENT_INSTR_RETIRED
    3, // PMU_EVENT_CACHE_REFERENCES (LLC Ref)
    4, // PMU_EVENT_CACHE_MISSES (LLC Misses)
    5, // PMU_EVENT_BRANCH_INSTRUCTIONS
    6  // PMU_EVENT_BRANCH_MISSES
};


int hal_pmu_init(void) {
    uint32_t eax=0, ebx=0, ecx=0, edx=0;
    x86_cpuid(CPUID_LEAF_PMU, &eax, &ebx, &ecx, &edx);

    pmu_version = eax & 0xFF;
    if (pmu_version == 0) {
        return 0; // PMU not supported (e.g., QEMU TCG), fallback to stub
    }

    num_general_counters = (eax >> 8) & 0xFF;
    if (num_general_counters > 8) num_general_counters = 8;

    unsupported_events_mask = ebx;
    supported_mask = PMU_EVENT_MASK_CYCLES; // CYCLES is always supported via RDTSC

    for (int i = 0; i < 8; i++) {
        pmc_in_use[i] = false;
    }
    for (int i = 0; i < 6; i++) {
        event_to_pmc[i] = -1;
    }

    // Try to map requested architectural events to general purpose counters
    int next_pmc = 0;
    for (int i = 1; i < 6; i++) { // Skip CYCLES, use RDTSC
        int intel_bit = our_event_to_intel_bit[i];
        if ((unsupported_events_mask & (1U << intel_bit)) == 0) {
            if (next_pmc < num_general_counters) {
                event_to_pmc[i] = next_pmc;
                pmc_in_use[next_pmc] = true;
                next_pmc++;
                supported_mask |= (1U << i);
            }
        }
    }

    return 0;
}

int hal_pmu_init_cpu_local(uint32_t cpu_id) {
    (void)cpu_id;
    if (pmu_version == 0) return 0;

    // Disable all counters first
    if (pmu_version > 1) {
        x86_wrmsr(MSR_IA32_PERF_GLOBAL_CTRL, 0);
    }

    for (int i = 0; i < num_general_counters; i++) {
        x86_wrmsr(MSR_IA32_PERFEVTSEL0 + i, 0);
    }

    // Enable GLOBAL_CTRL for used PMCs
    if (pmu_version > 1) {
        uint64_t global_ctrl = 0;
        for (int i = 0; i < num_general_counters; i++) {
            if (pmc_in_use[i]) {
                global_ctrl |= (1ULL << i);
            }
        }
        if (global_ctrl != 0) {
            x86_wrmsr(MSR_IA32_PERF_GLOBAL_CTRL, global_ctrl);
        }
    }
    return 0;
}

int hal_pmu_enable_event(hal_pmu_event_t event) {
    if (event == PMU_EVENT_CYCLES) {
        return 0;
    }

    if (pmu_version == 0 || event > PMU_EVENT_BRANCH_MISSES) {
        return -1;
    }

    int pmc = event_to_pmc[event];
    if (pmc == -1) {
        return -1;
    }

    uint64_t evtsel = arch_events[event];
    x86_wrmsr(MSR_IA32_PERFEVTSEL0 + pmc, evtsel);
    return 0;
}

int hal_pmu_disable_event(hal_pmu_event_t event) {
    if (event == PMU_EVENT_CYCLES) {
        return 0;
    }

    if (pmu_version == 0 || event > PMU_EVENT_BRANCH_MISSES) {
        return 0;
    }

    int pmc = event_to_pmc[event];
    if (pmc != -1) {
        x86_wrmsr(MSR_IA32_PERFEVTSEL0 + pmc, 0);
    }
    return 0;
}

uint64_t hal_pmu_read_event(hal_pmu_event_t event) {
    if (event == PMU_EVENT_CYCLES) {
        return rdtsc_ordered();
    }

    if (pmu_version == 0 || event > PMU_EVENT_BRANCH_MISSES) {
        return 0;
    }

    int pmc = event_to_pmc[event];
    if (pmc != -1) {
        return x86_rdmsr(MSR_IA32_PMC0 + pmc);
    }

    return 0;
}

void hal_pmu_snapshot(hal_pmu_snapshot_t* snapshot) {
    if (!snapshot) return;

    snapshot->cycles = hal_pmu_read_event(PMU_EVENT_CYCLES);
    snapshot->instr_retired = hal_pmu_read_event(PMU_EVENT_INSTR_RETIRED);
    snapshot->cache_refs = hal_pmu_read_event(PMU_EVENT_CACHE_REFERENCES);
    snapshot->cache_misses = hal_pmu_read_event(PMU_EVENT_CACHE_MISSES);
    snapshot->branch_instrs = hal_pmu_read_event(PMU_EVENT_BRANCH_INSTRUCTIONS);
    snapshot->branch_misses = hal_pmu_read_event(PMU_EVENT_BRANCH_MISSES);

    snapshot->approximate = (pmu_version == 0) ? 1u : 0u;
    snapshot->supported_events_mask = supported_mask;
}
