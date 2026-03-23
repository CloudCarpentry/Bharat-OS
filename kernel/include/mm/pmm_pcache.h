#ifndef BHARAT_PMM_PCACHE_H
#define BHARAT_PMM_PCACHE_H

#include <stdint.h>
#include <stddef.h>
#include "mm.h"
#include "spinlock.h"
#include "atomic.h"

// Scoped to order-0 only for Phase 1
#define PMM_PCACHE_ORDER 0
#define PMM_PCACHE_HIGH 128U
#define PMM_PCACHE_LOW  64U
#define PMM_REFILL_BATCH 32U
#define PMM_DRAIN_BATCH 32U
#define PMM_REMOTE_BATCH 32U
#define PMM_INBOX_SIZE 256U // Max remote frees pending per core

// Remote Free Inbox (MPSC bounded queue)
typedef struct {
    phys_addr_t pages[PMM_INBOX_SIZE];
    uint32_t head;
    uint32_t tail;
    spinlock_t lock; // Simple spinlock for producers (MPSC)

    // Stats
    uint32_t enqueue_count;
    uint32_t enqueue_failures;
    uint32_t drain_batches;
    uint32_t drain_pages;
} pmm_remote_inbox_t;

// Per-core, per-NUMA-node order-0 magazine
typedef struct {
    phys_addr_t pages[PMM_PCACHE_HIGH];
    uint32_t count;

    // Stats
    uint32_t alloc_hits;
    uint32_t alloc_misses;
    uint32_t refill_count;
    uint32_t refill_pages;
    uint32_t local_frees;
    uint32_t drain_to_zone_count;
    uint32_t direct_zone_allocs;
    uint32_t direct_zone_frees;
} pmm_pcache_t;

// Per-core top-level structure
typedef struct {
    bool active;
    pmm_pcache_t node_caches[4]; // MAX_NUMA_NODES is 4 in pmm.c
    pmm_remote_inbox_t inbox;
} pmm_core_state_t;

extern pmm_core_state_t g_pmm_cores[256]; // Assuming MAX_CORES 256

void pmm_pcache_init_all(void);
void pmm_core_local_init(uint32_t core_id);
void pmm_drain_remote_frees(uint32_t core_id);

#endif // BHARAT_PMM_PCACHE_H
