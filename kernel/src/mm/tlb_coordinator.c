#include "../../include/hal/hal_tlb.h"
#include "../../include/mm/aspace.h"
#include "../../include/mm/mm_remote.h"
#include "../../include/bharat/cpu_local.h"
#include "../../include/hal/hal.h"
#include "../../include/urpc/urpc_bootstrap.h"
#include "../../include/kernel.h"

// Synchronous shootdown coordinator

static uint64_t tlb_collect_targets(address_space_t *aspace) {
    uint64_t mask = 0;
    for (uint32_t i = 0; i < MAX_CPUS; i++) {
        // Need to check if core is online and running this aspace
        // Assume core is online if cpu_id matches array index (for now just check all)
        if (g_cpu_locals[i].current_as == aspace || g_cpu_locals[i].current_as_id == aspace->object_id) {
            mask |= (1ULL << i);
        }
    }
    return mask;
}

static void tlb_shootdown_sync(address_space_t *aspace, tlb_scope_t scope, virt_addr_t va, size_t len) {
    if (!aspace || !active_hal_tlb) return;

    // Increment AS sequence
    uint64_t seq = __atomic_add_fetch(&aspace->tlb_seq, 1, __ATOMIC_SEQ_CST);
    uint32_t current_core = hal_cpu_get_id();

    // Snapshot target mask
    uint64_t target_mask = tlb_collect_targets(aspace);

    // Remove self from mask to handle local flush separately
    target_mask &= ~(1ULL << current_core);

    extern int urpc_is_ready(uint32_t);
    extern int urpc_bootstrap_send(uint32_t, uint64_t);

    // Send requests
    for (uint32_t i = 0; i < MAX_CPUS; i++) {
        if ((target_mask & (1ULL << i)) && urpc_is_ready(i)) {
            mm_mailbox_slot_t* mailbox = &g_mm_mailboxes[i];

            spin_lock(&mailbox->lock);

            // Wait for mailbox to be clear if there was a previous un-acked message
            // In a robust system, we would have a queue or proper URPC rings for messages.
            while (mailbox->valid) {
                spin_unlock(&mailbox->lock);
                hal_core_poll_event(); // cpu_relax equivalent
                spin_lock(&mailbox->lock);
            }

            mailbox->msg.type = MM_MSG_TLB_FLUSH;
            mailbox->msg.scope = scope;
            mailbox->msg.sender_core = current_core;
            mailbox->msg.as_id = aspace->object_id;
            mailbox->msg.va = (uint64_t)va;
            mailbox->msg.len = len;
            mailbox->msg.seq = seq;

            mailbox->req_seq++; // Inform remote that a new request is ready
            mailbox->valid = 1;

            spin_unlock(&mailbox->lock);

            uint64_t doorbell = ((uint64_t)URPC_MM_MAILBOX << 56) | 0;
            urpc_bootstrap_send(i, doorbell);
            if (hal_send_ipi_payload) {
                hal_send_ipi_payload(i, 0);
            }
        } else {
            // If URPC not ready but core is targeted, clear the mask bit so we don't wait forever
            target_mask &= ~(1ULL << i);
        }
    }

    // Process local flush
    if (g_cpu_locals[current_core].current_as == aspace || g_cpu_locals[current_core].current_as_id == aspace->object_id) {
        if (scope == TLB_SCOPE_PAGE && active_hal_tlb->flush_page_local) {
            active_hal_tlb->flush_page_local(va);
        } else if (scope == TLB_SCOPE_RANGE && active_hal_tlb->flush_range_local) {
            active_hal_tlb->flush_range_local(va, len);
        } else if ((scope == TLB_SCOPE_ASPACE || scope == TLB_SCOPE_ALL) && active_hal_tlb->flush_asid_local) {
            active_hal_tlb->flush_asid_local(aspace->object_id & 0xFFFF); // Simplified ASID logic
        } else if (active_hal_tlb->flush_all_local) {
            active_hal_tlb->flush_all_local();
        }
    }

    // Wait for ACKs
    for (uint32_t i = 0; i < MAX_CPUS; i++) {
        if (target_mask & (1ULL << i)) {
            mm_mailbox_slot_t* mailbox = &g_mm_mailboxes[i];
            // Spin until ACK sequence catches up to request sequence
            while (mailbox->valid || mailbox->ack_seq < mailbox->req_seq) {
                hal_core_poll_event(); // cpu_relax equivalent
            }
        }
    }
}

void hal_tlb_invalidate_page(address_space_t *aspace, virt_addr_t va) {
    tlb_shootdown_sync(aspace, TLB_SCOPE_PAGE, va, PAGE_SIZE);
}

void hal_tlb_invalidate_range(address_space_t *aspace, virt_addr_t start, size_t len) {
    tlb_shootdown_sync(aspace, TLB_SCOPE_RANGE, start, len);
}

void hal_tlb_invalidate_aspace(address_space_t *aspace) {
    tlb_shootdown_sync(aspace, TLB_SCOPE_ASPACE, 0, 0);
}

void hal_tlb_invalidate_all(void) {
    // Only use for global fallback, e.g. kernel mappings change
    for (uint32_t i = 0; i < MAX_CPUS; i++) {
        if (g_cpu_locals[i].current_as) {
             tlb_shootdown_sync(g_cpu_locals[i].current_as, TLB_SCOPE_ALL, 0, 0);
        }
    }
}
