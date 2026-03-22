#ifndef BHARAT_MM_TLB_INTERNAL_H
#define BHARAT_MM_TLB_INTERNAL_H

#include "mm/tlb.h"
#include "bharat/cpu_local.h"

// Per-CPU state for TLB stats
typedef struct tlb_cpu_state {
    vm_aspace_t *active_aspace;
    uint64_t active_asid;
    uint64_t last_seen_gen;
    uint64_t shootdowns_received;
    uint64_t local_flushes;
    uint64_t shootdowns_sent;
    uint64_t full_flushes;
    uint64_t aspace_flushes;
    uint64_t range_flushes;
    uint64_t page_flushes;
} tlb_cpu_state_t;

extern tlb_cpu_state_t g_tlb_cpu_state[MAX_CPUS];

// Optional arch hook contract (used by coordinator internal)
// hal_tlb.h has ops. We can use them directly or bridge them.

#endif // BHARAT_MM_TLB_INTERNAL_H
