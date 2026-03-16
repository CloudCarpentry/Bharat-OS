#include "../../include/mm/mm_remote.h"
#include "../../include/urpc/urpc_bootstrap.h"
#include "../../include/hal/hal.h"

void kernel_panic(const char *message);

// Remote TLB operations (fire-and-forget)
void mm_remote_tlb_flush(uint32_t target_core, uint64_t as_id, virt_addr_t va) {
    // KASSERT(!in_urpc_handler() || !core_is_rt()); // RT cores shouldn't initiate blocking/complex remote requests inside handlers
    // For MVP, pack MSG type (0x01 = URPC_MSG_TLB_SHOOTDOWN) and VA into a single 64-bit msg.
    // In a full implementation, `as_id` must be part of the message payload so the receiver
    // can verify if it still has that AS active.
    uint64_t msg = ((uint64_t)va & ~0xFFFULL) | (0x01 & 0xFFF);
    (void)as_id;

    // Check if the system is fully booted and urpc is ready before sending
    // Declare explicit binding since urpc_bootstrap.h might use different names.
    extern int urpc_is_ready(uint32_t);
    extern int urpc_bootstrap_send(uint32_t, uint64_t);

    if (target_core != hal_cpu_get_id() && urpc_is_ready(target_core)) {
        urpc_bootstrap_send(target_core, msg);
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
