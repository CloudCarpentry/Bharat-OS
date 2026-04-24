#ifndef BHARAT_MM_TLB_PENDING_H
#define BHARAT_MM_TLB_PENDING_H

#include <stdint.h>
#include <stdbool.h>

#define BHARAT_TLB_MAX_PENDING_PER_CORE 16

typedef struct {
    uint32_t request_id;    // Encoded (core, slot, generation)
    uint64_t target_mask;
    uint64_t ack_mask;
    uint64_t aspace_id;
    int in_use;
} tlb_pending_entry_t;

typedef struct {
    uint64_t requests_issued;
    uint64_t targets_total;
    uint64_t acks_received;
    uint64_t stale_acks;
    uint64_t allocation_failures;
    uint64_t fallback_count;
} tlb_pending_stats_t;

// Encodes request ID from component pieces.
uint32_t tlb_reqid_encode(uint32_t core_id, uint32_t slot, uint32_t generation);

// Decodes request ID back to components.
void tlb_reqid_decode(uint32_t reqid, uint32_t* core_id, uint32_t* slot, uint32_t* generation);

// Initializes the per-core pending structures (called early on boot per-core or globally).
void tlb_pending_init(void);

// Allocates a pending slot for the current core.
// Returns the allocated slot index on success, or -1 if full.
int tlb_pending_alloc(uint64_t aspace_id, uint64_t target_mask, uint32_t* out_reqid);

// Acknowledges a request by request ID.
// This is safe to call from any core when handling a response or checking mailbox completion.
void tlb_pending_ack(uint32_t reqid, uint32_t acking_core);

// Checks if a request has been fully acknowledged.
bool tlb_pending_is_complete(uint32_t current_core, int slot);

// Frees a previously allocated slot.
void tlb_pending_free(uint32_t current_core, int slot);

// Retrieve stats pointer for a given core
tlb_pending_stats_t* tlb_pending_get_stats(uint32_t core_id);

#endif // BHARAT_MM_TLB_PENDING_H
