#include "../../include/hal/hal_tlb.h"
#include "../../include/mm/aspace.h"
#include "../../include/mm/mm_remote.h"
#include "../../include/bharat/cpu_local.h"
#include "../../include/hal/hal.h"
#include "../../include/urpc/urpc_bootstrap.h"
#include "../../include/kernel.h"

// Bring in the generated definitions
#include "../../../services/monitor/generated/bharat_monitor_v1_types.h"
#include "../../../subsys/include/bharat/msg/transport.h"

// Transport for core mock
bharat_transport_t* transport_for_core(int core) {
    (void)core;
    return NULL;
}
extern int bharat_monitor_v1_call_tlb_invalidate(bharat_transport_t* t, int dst, const bharat_monitor_v1_TlbInvalidateReq_t* req, void* ctx);

typedef struct {
    uint64_t active_aspace_id;
    uint32_t tlb_generation;
} cpu_mm_state_t;

cpu_mm_state_t cpu_mm_state[MAX_CPUS];

static uint64_t tlb_collect_targets(address_space_t *aspace) {
    uint64_t mask = 0;
    for (uint32_t i = 0; i < MAX_CPUS; i++) {
        // Need to check if core is online and running this aspace
        // Assume core is online if cpu_id matches array index (for now just check all)
        if (cpu_mm_state[i].active_aspace_id == aspace->object_id) {
            mask |= (1ULL << i);
        }
    }
    return mask;
}

// Update the active aspace in the cpu_mm_state array
void update_core_active_aspace(uint32_t core_id, uint64_t aspace_id) {
    if (core_id < MAX_CPUS) {
        cpu_mm_state[core_id].active_aspace_id = aspace_id;
    }
}

// Send the TLB Invalidate using the Monitor service format over URPC
void vmm_send_tlb_invalidate(uint64_t aspace_id,
                             uint64_t va,
                             uint64_t len,
                             uint32_t type)
{
    bharat_monitor_v1_TlbInvalidateReq_t req = {0};

    req.aspace_id = aspace_id;
    req.va_start  = va;
    req.length    = len;
    req.type      = type;

    uint32_t current_core = hal_cpu_get_id();

    // The generation counter now lives on the address space, allowing proper monotonic sequence tracking globally
    address_space_t *as = NULL;
    if (cpu_mm_state[current_core].active_aspace_id == aspace_id) {
        as = g_cpu_locals[current_core].current_as;
    }

    // Fallback if not current
    if (!as) {
        for (int i=0; i<MAX_CPUS; i++) {
            if (g_cpu_locals[i].current_as && g_cpu_locals[i].current_as->object_id == aspace_id) {
                as = g_cpu_locals[i].current_as;
                break;
            }
        }
    }

    if (as) {
        req.generation = __atomic_add_fetch(&as->tlb_seq, 1, __ATOMIC_SEQ_CST);
    } else {
        cpu_mm_state[current_core].tlb_generation++;
        req.generation = cpu_mm_state[current_core].tlb_generation;
    }

    for (int core = 0; core < MAX_CPUS; core++) {

        if (core == current_core) continue;

        // Ensure we strictly target cores running the active aspace
        if (g_cpu_locals[core].current_as_id != aspace_id && cpu_mm_state[core].active_aspace_id != aspace_id)
            continue;

        bharat_transport_t* t = transport_for_core(core);
        if (t) {
             // In a real implementation this would actually encode and send, then synchronously await ACK
             bharat_monitor_v1_call_tlb_invalidate(
                t,
                core,
                &req,
                NULL
            );
        }
    }
}

int monitor_handle_tlb_invalidate(
    void* ctx,
    const bharat_monitor_v1_TlbInvalidateReq_t* req,
    bharat_monitor_v1_TlbInvalidateResp_t* resp)
{
    uint32_t current_core = hal_cpu_get_id();
    cpu_mm_state_t* cpu = &cpu_mm_state[current_core];

    // Ignore if not running this aspace
    if (cpu->active_aspace_id != req->aspace_id) {
        resp->status = 0;
        return 0;
    }

    // Ignore stale invalidations
    if (req->generation < cpu->tlb_generation) {
        resp->status = 0;
        return 0;
    }

    cpu->tlb_generation = req->generation;

    switch (req->type) {
        case 0: // page
            if (active_hal_tlb && active_hal_tlb->flush_page_local) {
                active_hal_tlb->flush_page_local(req->va_start);
            }
            break;

        case 1: // range
            if (active_hal_tlb && active_hal_tlb->flush_range_local) {
                active_hal_tlb->flush_range_local(req->va_start, req->length);
            }
            break;

        case 2: // full
            if (active_hal_tlb && active_hal_tlb->flush_all_local) {
                active_hal_tlb->flush_all_local();
            }
            break;
    }

    resp->status = 0;
    return 0;
}

static void tlb_shootdown_sync(address_space_t *aspace, tlb_scope_t scope, virt_addr_t va, size_t len) {
    if (!aspace || !active_hal_tlb) return;

    uint32_t current_core = hal_cpu_get_id();

    uint32_t type;
    switch(scope) {
        case TLB_SCOPE_PAGE: type = 0; break;
        case TLB_SCOPE_RANGE: type = 1; break;
        default: type = 2; break; // ASPACE/ALL
    }

    // Send the invalidation requests
    vmm_send_tlb_invalidate(aspace->object_id, va, len, type);

    // Process local flush
    if (cpu_mm_state[current_core].active_aspace_id == aspace->object_id || g_cpu_locals[current_core].current_as_id == aspace->object_id) {

        if (scope == TLB_SCOPE_PAGE && active_hal_tlb->flush_page_local) {
            active_hal_tlb->flush_page_local(va);
        } else if (scope == TLB_SCOPE_RANGE && active_hal_tlb->flush_range_local) {
            active_hal_tlb->flush_range_local(va, len);
        } else if ((scope == TLB_SCOPE_ASPACE || scope == TLB_SCOPE_ALL) && active_hal_tlb->flush_asid_local) {
            active_hal_tlb->flush_asid_local(aspace->object_id & 0xFFFF);
        } else if (active_hal_tlb->flush_all_local) {
            active_hal_tlb->flush_all_local();
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
    for (uint32_t i = 0; i < MAX_CPUS; i++) {
        if (g_cpu_locals[i].current_as) {
             tlb_shootdown_sync(g_cpu_locals[i].current_as, TLB_SCOPE_ALL, 0, 0);
        }
    }
}

// Mailbox processing loop relocated to central TLB authority layer
void vmm_process_urpc_messages(void) {
    uint32_t current_core = hal_cpu_get_id();
    mm_mailbox_slot_t* mailbox = &g_mm_mailboxes[current_core];

    uint32_t messages_processed = 0;
    while (1) {
        if (mailbox->valid) {
            if (mailbox->req_seq != mailbox->ack_seq) {
                if (mailbox->msg.type == MM_MSG_TLB_FLUSH) {
                    if (mailbox->msg.as_id == KERNEL_AS_ID || core_current_as_id() == mailbox->msg.as_id) {
                        if (mailbox->msg.scope == TLB_SCOPE_PAGE) {
                            if (active_hal_tlb && active_hal_tlb->flush_page_local) {
                                active_hal_tlb->flush_page_local((virt_addr_t)mailbox->msg.va);
                            }
                        } else if (mailbox->msg.scope == TLB_SCOPE_RANGE) {
                            if (active_hal_tlb && active_hal_tlb->flush_range_local) {
                                active_hal_tlb->flush_range_local((virt_addr_t)mailbox->msg.va, mailbox->msg.len);
                            }
                        } else if (mailbox->msg.scope == TLB_SCOPE_ASPACE || mailbox->msg.scope == TLB_SCOPE_ALL) {
                            if (active_hal_tlb && active_hal_tlb->flush_asid_local) {
                                active_hal_tlb->flush_asid_local(mailbox->msg.as_id & 0xFFFF);
                            }
                        }
                    }
                    mailbox->ack_seq = mailbox->req_seq;
                }
                mailbox->valid = 0;
            }
        }
        messages_processed++;
        if (messages_processed >= 100) break;
    }
}
