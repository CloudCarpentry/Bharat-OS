#include "../../../include/hal/hal_tlb.h"
#include "../../../include/mm/aspace.h"
#include "../../../include/mm/mm_remote.h"
#include "../../../include/bharat/cpu_local.h"
#include "../../../include/hal/hal.h"
#include "../../../include/urpc/urpc_bootstrap.h"
#include "../../../include/kernel.h"
#include "../../../include/arch/cpu_relax.h"
#include "../../../include/panic.h"
#include "../../../include/bharat/console.h"
#include "../../../include/spinlock.h"

// Bring in the generated definitions
#include "bharat_monitor_v1_types.h"
#include "../../../../services/core/subsysmgr/include/bharat/msg/transport.h"
#include "../../../../services/core/subsysmgr/include/bharat/msg/wire.h"

// Optional transport resolver hook for core->transport routing.
// Keep a weak local stub so kernel linking does not depend on a board/service
// implementation always being present.
__attribute__((weak)) bharat_transport_t* transport_for_core(int core) {
    (void)core;
    return NULL;
}
extern int bharat_monitor_v1_call_tlb_invalidate(bharat_transport_t* t, int dst, const bharat_monitor_v1_TlbInvalidateReq_t* req, void* ctx);

// Global pending request tracking
#define MAX_PENDING_TLB_REQUESTS 64

typedef struct {
    uint64_t request_id;
    uint64_t target_mask;
    uint64_t ack_mask;
    uint64_t aspace_id;
    int in_use;
} pending_tlb_request_t;

static pending_tlb_request_t g_pending_requests[MAX_PENDING_TLB_REQUESTS];
static spinlock_t g_pending_requests_lock;
static bool g_pending_requests_lock_init = false;

static uint64_t tlb_collect_targets(uint64_t aspace_id) {
    uint64_t mask = 0;
    for (uint32_t i = 0; i < MAX_CPUS; i++) {
        if (g_cpu_locals[i].current_as_id == aspace_id) {
            mask |= (1ULL << i);
        }
    }
    return mask;
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
    if (g_cpu_locals[current_core].current_as_id == aspace_id) {
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

    uint64_t target_mask = 0;
    if (as) {
        req.generation = __atomic_add_fetch(&as->tlb_gen, 1, __ATOMIC_SEQ_CST);
        target_mask = __atomic_load_n(&as->active_mask, __ATOMIC_ACQUIRE);
    } else {
        req.generation = 1;
        target_mask = tlb_collect_targets(aspace_id); // Fallback to basic loop scan
    }

    // Do not wait for self
    target_mask &= ~(1ULL << current_core);

    if (target_mask == 0) {
        return; // Nothing to do remotely
    }

    // Allocate tracking slot
    pending_tlb_request_t* slot = NULL;

    if (!g_pending_requests_lock_init) {
        spin_lock_init(&g_pending_requests_lock);
        g_pending_requests_lock_init = true;
    }

    spin_lock(&g_pending_requests_lock);
    for (int i = 0; i < MAX_PENDING_TLB_REQUESTS; i++) {
        if (!g_pending_requests[i].in_use) {
            slot = &g_pending_requests[i];
            slot->request_id = req.generation;
            slot->aspace_id = aspace_id;
            slot->target_mask = target_mask;
            slot->ack_mask = 0;
            slot->in_use = 1;
            break;
        }
    }
    spin_unlock(&g_pending_requests_lock);

    for (int core = 0; core < MAX_CPUS; core++) {

        if (core == current_core) continue;

        // Strictly target only CPUs tracked in the active mask
        if (!(target_mask & (1ULL << core))) continue;

        bharat_transport_t* t = transport_for_core(core);
        if (t) {
             // Dispatch without synchronously waiting inside the transport call
             bharat_monitor_v1_call_tlb_invalidate(
                t,
                core,
                &req,
                NULL
            );
        } else {
             // Fallback to legacy mailbox
             mm_mailbox_slot_t* mailbox = &g_mm_mailboxes[core];
             spin_lock(&mailbox->lock);
             mailbox->msg.type = MM_MSG_TLB_FLUSH;
             mailbox->msg.scope = (type == 0) ? TLB_SCOPE_PAGE : (type == 1) ? TLB_SCOPE_RANGE : TLB_SCOPE_ASPACE;
             mailbox->msg.sender_core = current_core;
             mailbox->msg.as_id = aspace_id;
             mailbox->msg.va = va;
             mailbox->msg.len = len;
             mailbox->msg.seq = req.generation;
             mailbox->valid = 1;
             mailbox->req_seq++;
             spin_unlock(&mailbox->lock);

             // notify
             extern void hal_send_ipi_payload(uint32_t target_core, uint64_t payload);
             hal_send_ipi_payload(core, 0);
        }
    }

    if (slot) {
        // Synchronous wait with timeout and retry policy
        #define BHARAT_TLB_ACK_TIMEOUT_LOOPS 1000000
        #define BHARAT_TLB_MAX_RETRIES 3

        uint32_t retry_count = 0;
        bool success = false;

        while (retry_count < BHARAT_TLB_MAX_RETRIES) {
            uint32_t wait_loops = 0;
            while (wait_loops < BHARAT_TLB_ACK_TIMEOUT_LOOPS) {
                uint64_t current_ack;
                spin_lock(&g_pending_requests_lock);
                current_ack = slot->ack_mask;
                spin_unlock(&g_pending_requests_lock);

                if ((current_ack & target_mask) == target_mask) {
                    success = true;
                    break; // All acks received
                }

                // Let CPU relax and process incoming messages
                arch_cpu_relax();
                extern void vmm_process_urpc_messages(void);
                vmm_process_urpc_messages(); // check if acks arrived
                wait_loops++;
            }

            if (success) break;
            retry_count++;

            // If we get here, it means we timed out. We should retry sending to missing cores.
            // For now, in MVP we just wait longer or effectively treat the retry loop as extended wait.
            // In a full implementation we would re-transmit the URPC message here to the un-acked cores.
        }

        if (!success) {
            // TIMEOUT path for revocation. We fail closed.
            extern void kernel_panic(const char*);
            kernel_panic("TLB Shootdown Timeout: Revocation failed, system isolated to prevent memory corruption.");
        }

        // Free slot
        spin_lock(&g_pending_requests_lock);
        slot->in_use = 0;
        spin_unlock(&g_pending_requests_lock);
    }
}

int monitor_handle_tlb_invalidate(
    void* ctx,
    const bharat_monitor_v1_TlbInvalidateReq_t* req,
    bharat_monitor_v1_TlbInvalidateResp_t* resp)
{
    uint32_t current_core = hal_cpu_get_id();
    const hal_tlb_caps_t *caps = hal_tlb_caps();

    // Ignore if not running this aspace
    if (g_cpu_locals[current_core].current_as_id != req->aspace_id) {
        resp->status = 0;
        return 0;
    }

    // Ignore stale invalidations via software epoch tracking
    // Currently relying on target_mask tracking at sender for correct boundaries,
    // but in a strict protocol we should track last seen generation per aspace locally.

    switch (req->type) {
        case 0: // page
            if (active_hal_tlb && caps && caps->supports_page_flush && active_hal_tlb->flush_page_local) {
                active_hal_tlb->flush_page_local(req->va_start);
            }
            break;

        case 1: // range
            if (active_hal_tlb && caps && caps->supports_range_flush && active_hal_tlb->flush_range_local) {
                active_hal_tlb->flush_range_local(req->va_start, req->length);
            } else if (active_hal_tlb && active_hal_tlb->flush_all_local) {
                active_hal_tlb->flush_all_local();
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

#include "../../include/arch/arch_caps.h"
#include "../../include/mm/tlb.h"
#include "../../include/mm/tlb_internal.h"
#include "../../include/hal/hal_tlb.h"
#include "../../include/hal/hal.h"

int tlb_invalidate_remote(vm_aspace_t *aspace, uintptr_t va, size_t len, tlb_inv_kind_t kind) {
    if (!aspace || !active_hal_tlb) return -1;

    uint32_t type;
    switch(kind) {
        case TLB_INV_PAGE: type = 0; break;
        case TLB_INV_RANGE: type = 1; break;
        default: type = 2; break; // ASPACE/ALL
    }

    arch_caps_t caps = arch_get_caps();
    if (arch_caps_test(caps, ARCH_CAP_SMP)) {
        vmm_send_tlb_invalidate(aspace->object_id, va, len, type);
        uint32_t current_core = hal_cpu_get_id();
        g_tlb_cpu_state[current_core].shootdowns_sent++;
    }

    return 0;
}

int tlb_invalidate_all(vm_aspace_t *aspace, uintptr_t va, size_t len, tlb_inv_kind_t kind) {
    if (!aspace || !active_hal_tlb) return -1;

    tlb_invalidate_remote(aspace, va, len, kind);
    tlb_invalidate_local(aspace, va, len, kind);
    return 0;
}

void vmm_process_urpc_messages(void) {
    uint32_t current_core = hal_cpu_get_id();

    // Process new transport messages
    bharat_transport_t* t = transport_for_core(current_core);
    if (t && t->ops && t->ops->recv) {
        uint8_t buffer[256];
        size_t rx_len = 0;
        uint32_t limit = 0;

        while (limit++ < 100) {
            if (t->ops->poll) t->ops->poll(t, 0); // non-blocking
            int ret = t->ops->recv(t, buffer, sizeof(buffer), &rx_len);
            if (ret != BHARAT_MSG_OK || rx_len == 0) break;

            bharat_msg_header_t hdr;
            if (bharat_msg_header_decode(buffer, rx_len, &hdr) == BHARAT_MSG_OK) {
                // Handle TlbInvalidate Request
                if (hdr.service_id == 1 && hdr.opcode == 3 && bharat_msg_is_request(hdr.flags)) {
                    if (rx_len >= BHARAT_MSG_HEADER_MIN_LEN + sizeof(bharat_monitor_v1_TlbInvalidateReq_t)) {
                        uint8_t* payload = buffer + BHARAT_MSG_HEADER_MIN_LEN;
                        bharat_monitor_v1_TlbInvalidateReq_t req;
                        req.aspace_id = bharat_load_le64(payload + 0);
                        req.va_start  = bharat_load_le64(payload + 8);
                        req.length    = bharat_load_le64(payload + 16);
                        req.type      = bharat_load_le32(payload + 24);
                        req.generation= bharat_load_le32(payload + 28);

                        bharat_monitor_v1_TlbInvalidateResp_t resp = {0};

                        // Local execute and dispatch
                        monitor_handle_tlb_invalidate(NULL, &req, &resp);

                        // Ensure operations complete before ACK
                        __asm__ volatile("": : :"memory");

                        // Send response back
                        bharat_msg_header_t tx_hdr = {0};
                        tx_hdr.version_major = BHARAT_MSG_VERSION_MAJOR;
                        tx_hdr.version_minor = BHARAT_MSG_VERSION_MINOR;
                        tx_hdr.header_len    = BHARAT_MSG_HEADER_MIN_LEN;
                        tx_hdr.service_id    = 1; // monitor_v1
                        tx_hdr.opcode        = 3; // OP_TLBINVALIDATE
                        tx_hdr.flags         = BHARAT_MSG_FLAG_RESPONSE;
                        tx_hdr.request_id    = hdr.request_id; // Match sequence
                        tx_hdr.dst_node      = hdr.src_node;
                        tx_hdr.total_len     = BHARAT_MSG_HEADER_MIN_LEN + sizeof(bharat_monitor_v1_TlbInvalidateResp_t);

                        uint8_t tx_buf[256];
                        if (bharat_msg_header_encode(&tx_hdr, tx_buf, sizeof(tx_buf)) == BHARAT_MSG_OK) {
                            bharat_store_le32(tx_buf + BHARAT_MSG_HEADER_MIN_LEN, resp.status);
                            if (t->ops->send) {
                                t->ops->send(t, tx_buf, tx_hdr.total_len);
                            }
                        }
                    }
                } else if (hdr.service_id == 1 && hdr.opcode == 3 && bharat_msg_is_response(hdr.flags)) {
                    // It's a response to us
                    uint64_t req_id = hdr.request_id;

                    if (!g_pending_requests_lock_init) {
                        spin_lock_init(&g_pending_requests_lock);
                        g_pending_requests_lock_init = true;
                    }
                    spin_lock(&g_pending_requests_lock);
                    for (int i = 0; i < MAX_PENDING_TLB_REQUESTS; i++) {
                        if (g_pending_requests[i].in_use && g_pending_requests[i].request_id == req_id) {
                            if (g_pending_requests[i].target_mask & (1ULL << hdr.src_node)) {
                                g_pending_requests[i].ack_mask |= (1ULL << hdr.src_node);
                            }
                            break;
                        }
                    }
                    spin_unlock(&g_pending_requests_lock);
                }
            }
        }
    }

    // Process legacy bootstrap g_mm_mailboxes (Fallback)
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

        // Also we must process acks from other cores for our requests using legacy mailbox
        if (!g_pending_requests_lock_init) {
            spin_lock_init(&g_pending_requests_lock);
            g_pending_requests_lock_init = true;
        }
        spin_lock(&g_pending_requests_lock);
        for (int i = 0; i < MAX_PENDING_TLB_REQUESTS; i++) {
            if (g_pending_requests[i].in_use) {
                for (int c = 0; c < MAX_CPUS; c++) {
                    if ((g_pending_requests[i].target_mask & (1ULL << c)) &&
                        !(g_pending_requests[i].ack_mask & (1ULL << c))) {

                        mm_mailbox_slot_t* m = &g_mm_mailboxes[c];
                        // If req_seq == ack_seq and seq == our request generation, it's acked
                        if (m->req_seq == m->ack_seq && m->msg.seq == g_pending_requests[i].request_id) {
                            g_pending_requests[i].ack_mask |= (1ULL << c);
                        }
                    }
                }
            }
        }
        spin_unlock(&g_pending_requests_lock);

        messages_processed++;
        if (messages_processed >= 10) break;
    }
}
