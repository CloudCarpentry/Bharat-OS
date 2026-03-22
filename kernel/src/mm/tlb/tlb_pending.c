#include "tlb_pending.h"
#include "../../../include/hal/hal.h"
#include "../../../include/bharat/cpu_local.h"

// 8 bits core, 8 bits slot, 16 bits generation
#define REQID_CORE_SHIFT 24
#define REQID_SLOT_SHIFT 16
#define REQID_GEN_MASK   0xFFFF

static tlb_pending_entry_t g_tlb_pending[MAX_CPUS][BHARAT_TLB_MAX_PENDING_PER_CORE];
static uint32_t g_tlb_gen[MAX_CPUS][BHARAT_TLB_MAX_PENDING_PER_CORE];
static tlb_pending_stats_t g_tlb_stats[MAX_CPUS];

uint32_t tlb_reqid_encode(uint32_t core_id, uint32_t slot, uint32_t generation) {
    return (core_id << REQID_CORE_SHIFT) | (slot << REQID_SLOT_SHIFT) | (generation & REQID_GEN_MASK);
}

void tlb_reqid_decode(uint32_t reqid, uint32_t* core_id, uint32_t* slot, uint32_t* generation) {
    *core_id = (reqid >> REQID_CORE_SHIFT) & 0xFF;
    *slot = (reqid >> REQID_SLOT_SHIFT) & 0xFF;
    *generation = reqid & REQID_GEN_MASK;
}

void tlb_pending_init(void) {
    for (int c = 0; c < MAX_CPUS; c++) {
        for (int s = 0; s < BHARAT_TLB_MAX_PENDING_PER_CORE; s++) {
            g_tlb_pending[c][s].in_use = 0;
            g_tlb_gen[c][s] = 1; // Start generation at 1
        }
    }
}

int tlb_pending_alloc(uint64_t aspace_id, uint64_t target_mask, uint32_t* out_reqid) {
    uint32_t core_id = hal_cpu_get_id();
    for (int s = 0; s < BHARAT_TLB_MAX_PENDING_PER_CORE; s++) {
        if (!g_tlb_pending[core_id][s].in_use) {
            g_tlb_pending[core_id][s].in_use = 1;
            g_tlb_pending[core_id][s].aspace_id = aspace_id;
            g_tlb_pending[core_id][s].target_mask = target_mask;
            g_tlb_pending[core_id][s].ack_mask = 0;

            uint32_t gen = g_tlb_gen[core_id][s]++;
            uint32_t reqid = tlb_reqid_encode(core_id, s, gen);
            g_tlb_pending[core_id][s].request_id = reqid;
            *out_reqid = reqid;

            g_tlb_stats[core_id].requests_issued++;

            // Count targets
            uint64_t mask = target_mask;
            while (mask) {
                if (mask & 1) g_tlb_stats[core_id].targets_total++;
                mask >>= 1;
            }
            return s;
        }
    }
    g_tlb_stats[core_id].allocation_failures++;
    return -1;
}

void tlb_pending_ack(uint32_t reqid, uint32_t acking_core) {
    uint32_t core_id, slot, gen;
    tlb_reqid_decode(reqid, &core_id, &slot, &gen);

    if (core_id >= MAX_CPUS || slot >= BHARAT_TLB_MAX_PENDING_PER_CORE) return;

    tlb_pending_entry_t* entry = &g_tlb_pending[core_id][slot];

    // Validate that it's in use and generation matches
    // Note: since this is atomic and bounded per requester, we use atomic OR on ack_mask
    if (__atomic_load_n(&entry->in_use, __ATOMIC_ACQUIRE) &&
        __atomic_load_n(&entry->request_id, __ATOMIC_ACQUIRE) == reqid) {

        uint64_t mask = (1ULL << acking_core);
        uint64_t prev = __atomic_fetch_or(&entry->ack_mask, mask, __ATOMIC_RELEASE);
        if (!(prev & mask)) {
            __atomic_add_fetch(&g_tlb_stats[core_id].acks_received, 1, __ATOMIC_RELAXED);
        }
    } else {
        __atomic_add_fetch(&g_tlb_stats[core_id].stale_acks, 1, __ATOMIC_RELAXED);
    }
}

bool tlb_pending_is_complete(uint32_t current_core, int slot) {
    tlb_pending_entry_t* entry = &g_tlb_pending[current_core][slot];
    uint64_t ack = __atomic_load_n(&entry->ack_mask, __ATOMIC_ACQUIRE);
    return (ack & entry->target_mask) == entry->target_mask;
}

void tlb_pending_free(uint32_t current_core, int slot) {
    tlb_pending_entry_t* entry = &g_tlb_pending[current_core][slot];
    __atomic_store_n(&entry->in_use, 0, __ATOMIC_RELEASE);
}

tlb_pending_stats_t* tlb_pending_get_stats(uint32_t core_id) {
    if (core_id >= MAX_CPUS) return NULL;
    return &g_tlb_stats[core_id];
}
