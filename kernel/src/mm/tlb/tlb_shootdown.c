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
#include "../../../include/bharat/urpc.h"
#include "tlb_pending.h"
#include "../../../include/arch/arch_caps.h"
#include "../../../include/mm/tlb.h"
#include "../../../include/mm/tlb_internal.h"
#include "../../../include/hal/hal_ipi.h"

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

// Forward declarations
extern void vmm_process_urpc_messages(void);
extern void cap_handle_delegate_req(uint64_t payload, uint32_t source_core);
extern void cap_handle_delegate_ack(uint64_t payload);
extern void cap_handle_revoke_req(uint64_t payload, uint32_t source_core);
extern void cap_handle_revoke_ack(uint64_t payload);

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
        // We still increment the global aspace sequence, but we use the pending table's reqid for URPC tracking.
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
    uint32_t reqid = 0;
    int slot = tlb_pending_alloc(aspace_id, target_mask, &reqid);

    // In fallback mode, we still send the shootdown but we wait sequentially without tracking
    // For this simple version, we encode our generation if we fail allocation
    uint32_t final_reqid = reqid;
    if (slot < 0) {
        final_reqid = tlb_reqid_encode(current_core, 0xFF, req.generation & 0xFFFF);
        tlb_pending_get_stats(current_core)->fallback_count++;
    }

    req.generation = final_reqid;

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
             mailbox->msg.seq = req.generation; // Use the generated reqid for sequence tracking
             mailbox->valid = 1;
             mailbox->req_seq++;
             spin_unlock(&mailbox->lock);

             // notify via proper HAL IPI API
             hal_ipi_send(core, HAL_IPI_TLB_SHOOTDOWN);
        }
    }

    if (slot >= 0) {
        // Synchronous wait with timeout and retry policy
        #define BHARAT_TLB_ACK_TIMEOUT_LOOPS 1000000
        #define BHARAT_TLB_MAX_RETRIES 3

        uint32_t retry_count = 0;
        bool success = false;

        while (retry_count < BHARAT_TLB_MAX_RETRIES) {
            uint32_t wait_loops = 0;
            while (wait_loops < BHARAT_TLB_ACK_TIMEOUT_LOOPS) {
                if (tlb_pending_is_complete(current_core, slot)) {
                    success = true;
                    break; // All acks received
                }

                // Let CPU relax and process incoming messages
                arch_cpu_relax();
                vmm_process_urpc_messages(); // check if acks arrived
                wait_loops++;
            }

            if (success) break;
            retry_count++;
        }

        if (!success) {
            // TIMEOUT path for revocation. We fail closed.
            kernel_panic("TLB Shootdown Timeout: Revocation failed, system isolated to prevent memory corruption.");
        }

        // Free slot
        tlb_pending_free(current_core, slot);
    } else {
        // Fallback waiting (spinning without slot state tracking, relying directly on transport ack if possible)
        // Here we can simply wait on mailboxes, or a small wait loop
        uint32_t wait_loops = 0;
        while (wait_loops < 1000000) {
            arch_cpu_relax();
            vmm_process_urpc_messages();
            wait_loops++;
        }
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
                    uint32_t req_id = hdr.request_id;
                    uint32_t acking_core = hdr.src_node;

                    tlb_pending_ack(req_id, acking_core);
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
        // Wait, for our requests, we need to check if the other core updated its ack_seq
        // to match our sent req_seq.
        // We do this by scanning all cores to see if their mailbox matched the encoded seq
        for (int c = 0; c < MAX_CPUS; c++) {
            if (c == current_core) continue;
            mm_mailbox_slot_t* m = &g_mm_mailboxes[c];
            if (m->req_seq == m->ack_seq && m->msg.sender_core == current_core) {
                // The target core processed a message from us.
                // We use m->msg.seq as the reqid.
                uint32_t reqid = m->msg.seq;
                uint32_t core_id, slot, gen;
                tlb_reqid_decode(reqid, &core_id, &slot, &gen);

                // Only ack if it's our request (we already checked sender_core but just to be sure)
                if (core_id == current_core && slot < BHARAT_TLB_MAX_PENDING_PER_CORE) {
                    tlb_pending_ack(reqid, c);
                }
            }
        }

        messages_processed++;
        if (messages_processed >= 10) break;
    }

    // Process capability delegations from URPC bootstrap ring (for simplicity since transport isn't fully routed here yet)
    for (int c = 0; c < MAX_CPUS; c++) {
        if (c == current_core) continue;
        uint64_t raw_msg;
        // Non-blocking drain of messages from core `c` that aren't dropped.
        // Note: Because cap_table_delegate and cap_table_revoke both use synchronous polling
        // with `urpc_bootstrap_recv` that discards unhandled messages, we only catch requests
        // here if they arrive while NOT in a synchronous spin. We handle it here safely.
        int limit = 10;
        // We must peek so we don't accidentally dequeue and drop a message someone else is polling for.
        // Since `urpc_bootstrap_recv` doesn't have a peek, we'll rely on the fact that synchronous
        // waiters only poll their *target* core, so they might drop messages from *other* cores.
        // But for our patch scope, we just process what we get and avoid dropping.
        // For safe integration without peeking, we just receive and handle DELEGATE requests.
        while (limit-- > 0 && urpc_bootstrap_recv(c, &raw_msg) == 0) {
            urpc_msg_type_t type;
            uint64_t payload;
            urpc_unpack_msg(raw_msg, &type, &payload);

            if (type == URPC_CAP_DELEGATE_REQ) {
                cap_handle_delegate_req(payload, c);
            } else if (type == URPC_CAP_DELEGATE_ACK) {
                // In case an ACK arrives here, we set the global ack_received flag.
                cap_handle_delegate_ack(payload);
            } else if (type == URPC_CAP_REVOKE) {
                cap_handle_revoke_req(payload, c);
            } else if (type == URPC_CAP_REVOKE_ACK) {
                cap_handle_revoke_ack(payload);
            } else {
                // If we get here, it's another message type. The prior behavior was to drop it
                // in synchronous loops, but since we are replacing the global processor,
                // dropping it here continues the existing behavior until a unified queue is added.
                // It is what the existing codebase expects in this bootstrap phase.
            }
        }
    }
}
