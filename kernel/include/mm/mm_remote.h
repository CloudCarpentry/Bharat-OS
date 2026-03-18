#ifndef BHARAT_MM_REMOTE_H
#define BHARAT_MM_REMOTE_H

#include <stdint.h>
#include <stddef.h>
#include "../../include/mm.h"
#include "mm_local.h"

// Operations that cross core boundaries via URPC (may be async)

typedef enum {
    MM_MSG_TLB_FLUSH = 1,
    MM_MSG_AS_JOIN   = 2,
    MM_MSG_AS_LEAVE  = 3,
} mm_msg_type_t;

typedef struct {
    uint16_t type;
    uint16_t scope; // tlb_scope_t
    uint32_t sender_core;
    uint64_t as_id;
    uint64_t va;
    uint64_t len;
    uint64_t seq; // Request sequence
} mm_urpc_tlb_msg_t;

#include "../../include/spinlock.h"

typedef struct {
    spinlock_t lock;
    volatile uint32_t req_seq;
    volatile uint32_t ack_seq;
    volatile uint32_t valid;
    mm_urpc_tlb_msg_t msg;
} mm_mailbox_slot_t;

#define URPC_MM_MAILBOX 0xFF

// Array of mailboxes (one for each possible target core)
extern mm_mailbox_slot_t g_mm_mailboxes[64];

// Remote TLB operations (fire-and-forget)
void mm_remote_tlb_flush(uint32_t target_core, uint64_t as_id, virt_addr_t va);
void mm_remote_tlb_shootdown_mask(uint64_t core_membership_mask, uint64_t as_id, virt_addr_t va);

// Remote AS operations
void mm_remote_as_join(uint32_t target_core, uint64_t as_id);
void mm_remote_as_leave(uint32_t target_core, uint64_t as_id);

// PMM Remote Operations (borrowing)
void mm_remote_pmm_borrow(uint32_t target_core, size_t n_pages);

// Slab Remote Operations (freeing)
void mm_remote_slab_free(uint32_t target_core, void* obj_ptr, uint32_t cache_id);

// Fault coordination
void mm_remote_as_lock(uint32_t target_core, uint64_t as_id);
void mm_remote_stack_setup(uint32_t target_core, virt_addr_t stack_va, virt_addr_t guard_va, size_t size);

#endif // BHARAT_MM_REMOTE_H