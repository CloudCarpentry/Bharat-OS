
#include "mm/mem_model.h"
#include "hal/hal_tlb.h"
#include "hal/hal_ipi.h"
#include "hal/hal.h"
#include "bharat/cpu_local.h"
#include "mm/tlb_internal.h"
#include "mm/aspace.h"
#include "mm/mm_remote.h"
#include "mm/tlb.h"
#include "arch/arch_caps.h"
#include "arch/cpu_relax.h"
#include "tlb_pending.h"
#include "panic.h"
#include "bharat/console.h"
#include "urpc/urpc_bootstrap.h"
#include "bharat/urpc.h"
#include "kernel.h"

// Bring in the generated definitions
#ifndef BHARAT_HOST_TEST
#include "bharat_monitor_v1_types.h"
#include "bharat/msg/transport.h"
#include "bharat/msg/wire.h"
#else
// Minimal mocks for host tests
typedef struct {
    uint64_t aspace_id;
    uint64_t va_start;
    uint64_t length;
    uint32_t type;
    uint32_t generation;
} bharat_monitor_v1_TlbInvalidateReq_t;

typedef struct {
    uint32_t status;
} bharat_monitor_v1_TlbInvalidateResp_t;

typedef struct {
    int dummy;
    struct {
        int (*send)(void* t, const void* buf, size_t len);
        int (*recv)(void* t, void* buf, size_t max, size_t* rx_len);
        int (*poll)(void* t, int timeout);
    }* ops;
} bharat_transport_t;

#define BHARAT_MSG_HEADER_MIN_LEN 16
#define BHARAT_MSG_OK 0
#define BHARAT_MSG_VERSION_MAJOR 1
#define BHARAT_MSG_VERSION_MINOR 0
#define BHARAT_MSG_FLAG_RESPONSE 1

typedef struct {
    uint8_t version_major;
    uint8_t version_minor;
    uint8_t header_len;
    uint8_t flags;
    uint16_t service_id;
    uint16_t opcode;
    uint32_t request_id;
    uint32_t src_node;
    uint32_t dst_node;
    uint32_t total_len;
} bharat_msg_header_t;

static inline int bharat_msg_header_decode(const void* buf, size_t len, bharat_msg_header_t* hdr) { return -1; }
static inline int bharat_msg_header_encode(const bharat_msg_header_t* hdr, void* buf, size_t max) { return -1; }
static inline bool bharat_msg_is_request(uint8_t flags) { return false; }
static inline bool bharat_msg_is_response(uint8_t flags) { return (flags & BHARAT_MSG_FLAG_RESPONSE) != 0; }
static inline uint64_t bharat_load_le64(const void* p) { return 0; }
static inline uint32_t bharat_load_le32(const void* p) { return 0; }
static inline void bharat_store_le32(void* p, uint32_t v) {}
#endif

// Optional transport resolver hook for core->transport routing.
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

static kstatus_t tlb_send_via_transport(uint32_t core, const bharat_monitor_v1_TlbInvalidateReq_t* req) {
    bharat_transport_t* t = transport_for_core(core);
    if (t) {
         return (kstatus_t)bharat_monitor_v1_call_tlb_invalidate(t, core, req, NULL);
    }
    return K_ERR_NOT_FOUND;
}

static void tlb_send_via_mailbox_legacy(uint32_t core, uint32_t current_core, uint32_t type, vm_aspace_t* aspace, uint64_t va, uint64_t len, uint32_t generation) {
    mm_mailbox_slot_t* mailbox = &g_mm_mailboxes[core];
    spin_lock(&mailbox->lock);
    mailbox->msg.type = MM_MSG_TLB_FLUSH;
    mailbox->msg.scope = (type == 0) ? TLB_SCOPE_PAGE : (type == 1) ? TLB_SCOPE_RANGE : TLB_SCOPE_ASPACE;
    mailbox->msg.sender_core = current_core;
    mailbox->msg.as_id = aspace ? aspace->object_id : 0;
    mailbox->msg.va = va;
    mailbox->msg.len = len;
    mailbox->msg.seq = generation;
    mailbox->valid = 1;
    mailbox->req_seq++;
    spin_unlock(&mailbox->lock);

    hal_ipi_send(core, HAL_IPI_TLB_SHOOTDOWN);
}

static kstatus_t tlb_wait_for_acks_bounded(uint32_t current_core, int slot) {
    #define BHARAT_TLB_ACK_TIMEOUT_LOOPS 1000000
    #define BHARAT_TLB_MAX_RETRIES 3

    uint32_t retry_count = 0;
    while (retry_count < BHARAT_TLB_MAX_RETRIES) {
        uint32_t wait_loops = 0;
        while (wait_loops < BHARAT_TLB_ACK_TIMEOUT_LOOPS) {
            if (tlb_pending_is_complete(current_core, slot)) {
                return K_OK;
            }
            arch_cpu_relax();
            vmm_process_urpc_messages();
            wait_loops++;
        }
        retry_count++;
    }
    return K_ERR_TIMEOUT;
}

static void tlb_handle_failure(tlb_failure_policy_t policy, uint64_t aspace_id, uint32_t reqid) {
    uint32_t core = hal_cpu_get_id();
    console_log(CONSOLE_LEVEL_PANIC,
        "TLB: Shootdown failure! core=%u aspace=%lu reqid=0x%x policy=%d\n",
        core, aspace_id, reqid, (int)policy);

    switch (policy) {
        case TLB_FAIL_KERNEL_PANIC:
            kernel_panic("TLB Shootdown Timeout: Revocation failed. System halted to prevent corruption.");
            break;
        case TLB_FAIL_ISOLATE_ASPACE: {
            // Find the aspace object to mark it poisoned.
            // In a real system we'd look it up by ID if we don't have the pointer.
            // For now we assume we are called from a context where we can find it.
            // Since we don't have a global aspace lookup by ID easily accessible here,
            // we will need to rely on the caller to handle isolation if possible,
            // or we implement a basic lookup.
            // Given the instruction, we'll mark aspace poisoned in the caller.
            break;
        }
        default:
            break;
    }
}

static tlb_failure_policy_t tlb_default_failure_policy(void) {
#if defined(BHARAT_PROFILE_RTOS) || defined(BHARAT_PROFILE_SAFETY)
    return TLB_FAIL_ISOLATE_ASPACE;
#elif defined(BHARAT_KERNEL_HARDENING_FATAL)
    return TLB_FAIL_KERNEL_PANIC;
#else
    return TLB_FAIL_RETURN_ERROR;
#endif
}

static bool tlb_failure_policy_is_fatal(tlb_failure_policy_t policy) {
    return policy == TLB_FAIL_KERNEL_PANIC;
}

kstatus_t vmm_send_tlb_invalidate_ex(vm_aspace_t *aspace,
                                uint64_t va,
                                uint64_t len,
                                uint32_t type,
                                tlb_failure_policy_t failure_policy)
{
    if (!aspace_is_valid_for_tlb(aspace)) return K_ERR_INVALID_ARG;

    bharat_monitor_v1_TlbInvalidateReq_t req = {0};
    req.aspace_id = aspace->object_id;
    req.va_start  = va;
    req.length    = len;
    req.type      = type;

    uint32_t current_core = hal_cpu_get_id();
    uint64_t target_mask = aspace_get_active_mask(aspace);

    // Do not wait for self
    target_mask &= ~(1ULL << current_core);

    if (target_mask == 0) {
        return K_OK;
    }

    uint32_t reqid = 0;
    int slot = tlb_pending_alloc(aspace->object_id, target_mask, &reqid);

    if (slot < 0) {
        // Fallback reqid
        reqid = tlb_reqid_encode(current_core, 0xFF, (uint32_t)aspace->tlb_gen & 0xFFFF);
        tlb_pending_get_stats(current_core)->fallback_count++;
    }

    req.generation = reqid;
    (void)aspace_next_tlb_generation(aspace);

    bool any_sent = false;
    for (int core = 0; core < MAX_CPUS; core++) {
        if (core == current_core) continue;
        if (!(target_mask & (1ULL << core))) continue;

        if (tlb_send_via_transport(core, &req) == K_OK) {
             any_sent = true;
        } else {
             // KERN-P0-002: Disable legacy mailbox by default for hardened profiles
             #ifndef BHARAT_DISABLE_LEGACY_TLB_MAILBOX
             tlb_send_via_mailbox_legacy(core, current_core, type, aspace, va, len, req.generation);
             any_sent = true;
             #endif
        }
    }

    if (!any_sent) {
        if (slot >= 0) tlb_pending_free(current_core, slot);
        return K_ERR_NOT_FOUND;
    }

    kstatus_t status = K_OK;
    if (slot >= 0) {
        status = tlb_wait_for_acks_bounded(current_core, slot);
        if (status != K_OK) {
            tlb_handle_failure(failure_policy, aspace->object_id, reqid);
            if (failure_policy == TLB_FAIL_ISOLATE_ASPACE) {
                aspace_mark_poisoned(aspace);
            }
        }
        tlb_pending_free(current_core, slot);
    } else {
        // We no longer allow ACK-less success. If we couldn't allocate a slot,
        // we must fail or we must have a pre-allocated emergency slot.
        // For now, return error.
        status = K_ERR_QUOTA_EXCEEDED;
    }

    return status;
}

void vmm_send_tlb_invalidate(vm_aspace_t *aspace,
                             uint64_t va,
                             uint64_t len,
                             uint32_t type)
{
    vmm_send_tlb_invalidate_ex(aspace, va, len, type, tlb_default_failure_policy());
}

int monitor_handle_tlb_invalidate(
    void* ctx,
    const bharat_monitor_v1_TlbInvalidateReq_t* req,
    bharat_monitor_v1_TlbInvalidateResp_t* resp)
{
    (void)ctx;
    uint32_t current_core = hal_cpu_get_id();
    const hal_tlb_caps_t *caps = hal_tlb_caps();

    // Ignore if not running this aspace
    if (g_cpu_locals[current_core].current_as_id != req->aspace_id) {
        resp->status = 0;
        return 0;
    }

    switch (req->type) {
        case 0: // page
            hal_tlb_invalidate_local_page(req->va_start);
            break;

        case 1: // range
            hal_tlb_invalidate_local_range(req->va_start, req->length);
            break;

        case 2: // full
            hal_tlb_invalidate_local_aspace(req->aspace_id);
            break;
    }

    resp->status = 0;
    return 0;
}

int tlb_invalidate_remote_ex(vm_aspace_t *aspace, uintptr_t va, size_t len, tlb_inv_kind_t kind, tlb_failure_policy_t failure_policy) {
    if (!aspace || !active_hal_tlb) return -1;

    uint32_t type;
    switch(kind) {
        case TLB_INV_PAGE: type = 0; break;
        case TLB_INV_RANGE: type = 1; break;
        default: type = 2; break; // ASPACE/ALL
    }

    arch_caps_t caps = arch_get_caps();
    if (arch_caps_test(caps, ARCH_CAP_SMP)) {
        kstatus_t status = vmm_send_tlb_invalidate_ex(aspace, va, len, type, failure_policy);
        uint32_t current_core = hal_cpu_get_id();
        g_tlb_cpu_state[current_core].shootdowns_sent++;
        if (status != K_OK) return -1;
    }

    return 0;
}

int tlb_invalidate_remote(vm_aspace_t *aspace, uintptr_t va, size_t len, tlb_inv_kind_t kind) {
    return tlb_invalidate_remote_ex(aspace, va, len, kind, TLB_FAIL_RETURN_ERROR);
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
    bharat_transport_t* t = transport_for_core((int)current_core);
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
    if (mailbox->valid) {
        // Handle mailbox message
        if (mailbox->msg.type == MM_MSG_TLB_FLUSH) {
             bharat_monitor_v1_TlbInvalidateReq_t req;
             req.aspace_id = mailbox->msg.as_id;
             req.va_start = mailbox->msg.va;
             req.length = mailbox->msg.len;
             req.type = (mailbox->msg.scope == TLB_SCOPE_PAGE) ? 0 : (mailbox->msg.scope == TLB_SCOPE_RANGE) ? 1 : 2;
             req.generation = mailbox->msg.seq;

             bharat_monitor_v1_TlbInvalidateResp_t resp = {0};
             monitor_handle_tlb_invalidate(NULL, &req, &resp);

             // ACK via the new protocol even if requested via legacy mailbox
             tlb_pending_ack(req.generation, current_core);
        }
        spin_lock(&mailbox->lock);
        mailbox->valid = 0;
        spin_unlock(&mailbox->lock);
    }

    // Process capability delegations from URPC bootstrap ring
    for (int c = 0; c < MAX_CPUS; c++) {
        if (c == (int)current_core) continue;
        uint64_t raw_msg;
        int limit = 10;
        while (limit-- > 0 && urpc_bootstrap_recv(c, &raw_msg) == 0) {
            urpc_msg_type_t type;
            uint64_t payload;
            urpc_unpack_msg(raw_msg, &type, &payload);

            if (type == URPC_CAP_DELEGATE_REQ) {
                cap_handle_delegate_req(payload, (uint32_t)c);
            } else if (type == URPC_CAP_DELEGATE_ACK) {
                cap_handle_delegate_ack(payload);
            } else if (type == URPC_CAP_REVOKE) {
                cap_handle_revoke_req(payload, (uint32_t)c);
            } else if (type == URPC_CAP_REVOKE_ACK) {
                cap_handle_revoke_ack(payload);
            }
        }
    }
}
