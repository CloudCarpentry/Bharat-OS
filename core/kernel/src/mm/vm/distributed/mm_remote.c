#include "../../../../include/mm.h"
#include "../../../../include/mm/vm_space.h"
#include "../../../../include/mm/vm_mapping.h"
#include "../../../../include/monitor/mon_vm_ops.h"
#include "../../../../include/hal/hal.h"
#include "../../../../include/slab.h"
#include "../../../../include/mm/mm_remote.h"

void kernel_panic(const char *message);

mm_mailbox_slot_t g_mm_mailboxes[64];

void init_mm_mailboxes(void) {
    for (int i = 0; i < 64; i++) {
        spin_lock_init(&g_mm_mailboxes[i].lock);
    }
}

// Remote TLB operations (fire-and-forget)
void mm_remote_tlb_flush(uint32_t target_core, uint64_t as_id, virt_addr_t va) {
    // KASSERT(!in_urpc_handler() || !core_is_rt()); // RT cores shouldn't initiate blocking/complex remote requests inside handlers

    // Check if the system is fully booted and urpc is ready before sending
    // Declare explicit binding since urpc_bootstrap.h might use different names.
    extern int urpc_is_ready(uint32_t);
    extern int urpc_bootstrap_send(uint32_t, uint64_t);

    uint32_t current_core = hal_cpu_get_id();

    if (target_core < 64 && target_core != current_core && urpc_is_ready(target_core)) {
        // Prepare structured mailbox payload
        mm_mailbox_slot_t* mailbox = &g_mm_mailboxes[target_core];

        // Setup message
        mailbox->msg.type = MM_MSG_TLB_FLUSH;
        mailbox->msg.sender_core = current_core;
        mailbox->msg.as_id = as_id;
        mailbox->msg.va = (uint64_t)va;

        mailbox->valid = 1;
        mailbox->req_seq++;

        // Send doorbell through bootstrap ring. We encode the type as URPC_MM_MAILBOX
        // and payload could just be the target core, but for now we just pass 0
        // since the receiver just reads its own local mailbox slot.
        // We pack URPC_MM_MAILBOX as the type, payload is 0
        uint64_t doorbell = ((uint64_t)URPC_MM_MAILBOX << 56) | 0;

        urpc_bootstrap_send(target_core, doorbell);
        hal_send_ipi_payload(target_core, 0); // notify
    }
}

// Global TLB shootdown that respects core membership
void mm_remote_tlb_shootdown_mask(uint64_t core_membership_mask, uint64_t as_id, virt_addr_t va) {
    uint32_t current_core = hal_cpu_get_id();
    for (uint32_t i = 0; i < 64; i++) {
        if ((core_membership_mask & (1ULL << i)) && (i != current_core)) {
            mm_remote_tlb_flush(i, as_id, va);
        }
    }
}

// Remote AS operations
void mm_remote_as_join(uint32_t target_core, uint64_t as_id) {
    (void)target_core;
    (void)as_id;
    // Stub
}

void mm_remote_as_leave(uint32_t target_core, uint64_t as_id) {
    (void)target_core;
    (void)as_id;
    // Stub
}

// PMM Remote Operations (borrowing)
void mm_remote_pmm_borrow(uint32_t target_core, size_t n_pages) {
    if (core_is_rt()) {
        kernel_panic("RT core attempted to borrow physical memory remotely!");
    }
    (void)target_core;
    (void)n_pages;
    // Stub: sends MSG_PMM_BORROW
}

// Slab Remote Operations (freeing)
void mm_remote_slab_free(uint32_t target_core, void* obj_ptr, uint32_t cache_id) {
    (void)target_core;
    (void)obj_ptr;
    (void)cache_id;
    // Stub: sends MSG_SLAB_REMOTE_FREE
}

// Fault coordination
void mm_remote_as_lock(uint32_t target_core, uint64_t as_id) {
    if (core_is_rt()) {
        kernel_panic("RT core attempted to remotely lock AS for CoW!");
    }
    (void)target_core;
    (void)as_id;
    // Stub
}

void mm_remote_stack_setup(uint32_t target_core, virt_addr_t stack_va, virt_addr_t guard_va, size_t size) {
    (void)target_core;
    (void)stack_va;
    (void)guard_va;
    (void)size;
    // Stub
}
