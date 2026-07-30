#include "mm/mem_model.h"
#include "hal/hal_tlb.h"
#include "hal/hal_ipi.h"
#include "hal/hal.h"
#include "hal/hal_timer.h"
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

// Central fallback policy configuration
#ifndef BHARAT_TLB_LEGACY_MAILBOX_FALLBACK
  #if defined(BHARAT_PROFILE_MMU_FULL) || defined(BHARAT_PROFILE_RTOS) || defined(BHARAT_PROFILE_SAFETY) || defined(BHARAT_KERNEL_HARDENING_FATAL)
    #define BHARAT_TLB_LEGACY_MAILBOX_FALLBACK 0
  #else
    #define BHARAT_TLB_LEGACY_MAILBOX_FALLBACK 1
  #endif
#endif

// Bring in the generated definitions
#ifndef BHARAT_HOST_TEST
#include "bharat_monitor_v1_types.h"
#include "bharat/msg/transport.h"
#include "bharat/msg/wire.h"
#else
// Minimal mocks for host tests
#include <bharat/uapi/subsys/msg_types.h>

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

typedef struct bharat_transport {
    const struct {
        int (*send)(struct bharat_transport* self, const uint8_t* buf, size_t len);
        int (*recv)(struct bharat_transport* self, uint8_t* buf, size_t cap, size_t* out_len);
        uint32_t (*get_caps)(struct bharat_transport* self);
        size_t (*get_mtu)(struct bharat_transport* self);
        int (*poll)(struct bharat_transport* self, int timeout_ms);
    }* ops;
    void* ctx;
    uint32_t local_id;
} bharat_transport_t;

#define BHARAT_MSG_HEADER_MIN_LEN 44
#define BHARAT_MSG_MAGIC         0x42485254  // "BHRT"
#define BHARAT_MSG_OK 0
#define BHARAT_MSG_VERSION_MAJOR 1
#define BHARAT_MSG_VERSION_MINOR 0
#define BHARAT_MSG_FLAG_REQUEST      (1U << 0)
#define BHARAT_MSG_FLAG_RESPONSE     (1U << 1)

typedef struct {
    uint8_t  version_major;
    uint8_t  version_minor;
    uint16_t header_len;
    uint16_t service_id;
    uint16_t opcode;
    uint32_t flags;
    uint32_t total_len;
    uint64_t request_id;
    uint32_t src_node;
    uint32_t dst_node;
    uint16_t cap_count;
    uint16_t desc_count;
    uint32_t header_crc;
} bharat_msg_header_t;

static inline int bharat_msg_header_decode(const void* buf, size_t len, bharat_msg_header_t* hdr) {
    if (!buf || !hdr) return -1;
    if (len < BHARAT_MSG_HEADER_MIN_LEN) return -1;
    uint32_t magic = bharat_load_le32((const uint8_t*)buf + 0x00);
    if (magic != BHARAT_MSG_MAGIC) return -1;
    hdr->version_major = ((const uint8_t*)buf)[0x04];
    hdr->version_minor = ((const uint8_t*)buf)[0x05];
    hdr->header_len    = bharat_load_le16((const uint8_t*)buf + 0x06);
    hdr->service_id    = bharat_load_le16((const uint8_t*)buf + 0x08);
    hdr->opcode        = bharat_load_le16((const uint8_t*)buf + 0x0A);
    hdr->flags         = bharat_load_le32((const uint8_t*)buf + 0x0C);
    hdr->total_len     = bharat_load_le32((const uint8_t*)buf + 0x10);
    hdr->request_id    = bharat_load_le64((const uint8_t*)buf + 0x14);
    hdr->src_node      = bharat_load_le32((const uint8_t*)buf + 0x1C);
    hdr->dst_node      = bharat_load_le32((const uint8_t*)buf + 0x20);
    hdr->cap_count     = bharat_load_le16((const uint8_t*)buf + 0x24);
    hdr->desc_count    = bharat_load_le16((const uint8_t*)buf + 0x26);
    hdr->header_crc    = bharat_load_le32((const uint8_t*)buf + 0x28);
    return BHARAT_MSG_OK;
}

static inline int bharat_msg_header_encode(const bharat_msg_header_t* hdr, void* buf, size_t max) {
    if (!hdr || !buf) return -1;
    if (max < hdr->header_len || max < BHARAT_MSG_HEADER_MIN_LEN) return -1;
    bharat_store_le32((uint8_t*)buf + 0x00, BHARAT_MSG_MAGIC);
    ((uint8_t*)buf)[0x04] = hdr->version_major;
    ((uint8_t*)buf)[0x05] = hdr->version_minor;
    bharat_store_le16((uint8_t*)buf + 0x06, hdr->header_len);
    bharat_store_le16((uint8_t*)buf + 0x08, hdr->service_id);
    bharat_store_le16((uint8_t*)buf + 0x0A, hdr->opcode);
    bharat_store_le32((uint8_t*)buf + 0x0C, hdr->flags);
    bharat_store_le32((uint8_t*)buf + 0x10, hdr->total_len);
    bharat_store_le64((uint8_t*)buf + 0x14, hdr->request_id);
    bharat_store_le32((uint8_t*)buf + 0x1C, hdr->src_node);
    bharat_store_le32((uint8_t*)buf + 0x20, hdr->dst_node);
    bharat_store_le16((uint8_t*)buf + 0x24, hdr->cap_count);
    bharat_store_le16((uint8_t*)buf + 0x26, hdr->desc_count);
    bharat_store_le32((uint8_t*)buf + 0x28, hdr->header_crc);
    return BHARAT_MSG_OK;
}

static inline bool bharat_msg_is_request(uint32_t flags) { return (flags & BHARAT_MSG_FLAG_REQUEST) != 0; }
static inline bool bharat_msg_is_response(uint32_t flags) { return (flags & BHARAT_MSG_FLAG_RESPONSE) != 0; }
#endif

typedef struct {
    volatile uint32_t request_id;
    volatile uint32_t status;
    volatile uint32_t acking_core;
    volatile uint32_t valid;
} tlb_legacy_resp_t;

static tlb_legacy_resp_t g_tlb_legacy_resps[MAX_CPUS][BHARAT_TLB_MAX_PENDING_PER_CORE];

// Optional transport resolver hook for core->transport routing.
__attribute__((weak)) bharat_transport_t* transport_for_core(int core) {
    (void)core;
    return NULL;
}
extern int bharat_monitor_v1_send_tlb_invalidate(bharat_transport_t* t, int dst, const bharat_monitor_v1_TlbInvalidateReq_t* req, uint64_t reqid, void* ctx);

// Forward declarations
extern void vmm_process_urpc_messages(void);

void tlb_send_completion(uint32_t request_id, uint32_t origin_cpu, uint32_t status) {
    uint32_t core_id, slot, gen;
    tlb_reqid_decode(request_id, &core_id, &slot, &gen);

    if (core_id >= MAX_CPUS || slot >= BHARAT_TLB_MAX_PENDING_PER_CORE) return;

    bharat_transport_t* t = transport_for_core((int)origin_cpu);
    if (t) {
        // Normal transport available -> send response message
        bharat_msg_header_t tx_hdr = {0};
        tx_hdr.version_major = BHARAT_MSG_VERSION_MAJOR;
        tx_hdr.version_minor = BHARAT_MSG_VERSION_MINOR;
        tx_hdr.header_len    = BHARAT_MSG_HEADER_MIN_LEN;
        tx_hdr.service_id    = 1; // monitor_v1
        tx_hdr.opcode        = 3; // OP_TLBINVALIDATE
        tx_hdr.flags         = BHARAT_MSG_FLAG_RESPONSE;
        tx_hdr.request_id    = request_id;
        tx_hdr.src_node      = hal_cpu_get_id();
        tx_hdr.dst_node      = origin_cpu;
        tx_hdr.total_len     = BHARAT_MSG_HEADER_MIN_LEN + sizeof(bharat_monitor_v1_TlbInvalidateResp_t);

        uint8_t tx_buf[256];
        if (bharat_msg_header_encode(&tx_hdr, tx_buf, sizeof(tx_buf)) == BHARAT_MSG_OK) {
            bharat_store_le32(tx_buf + BHARAT_MSG_HEADER_MIN_LEN, status);
            if (t->ops && t->ops->send) {
                t->ops->send(t, tx_buf, tx_hdr.total_len);
            }
        }
    } else {
        // Legacy mailbox fallback response queue
        g_tlb_legacy_resps[origin_cpu][slot].request_id = request_id;
        g_tlb_legacy_resps[origin_cpu][slot].status = status;
        g_tlb_legacy_resps[origin_cpu][slot].acking_core = hal_cpu_get_id();
        __atomic_store_n(&g_tlb_legacy_resps[origin_cpu][slot].valid, 1, __ATOMIC_RELEASE);

        // IPI notification doorbell
        hal_ipi_send(origin_cpu, HAL_IPI_TLB_SHOOTDOWN);
    }
}
extern void cap_handle_delegate_req(uint64_t payload, uint32_t source_core);
extern void cap_handle_delegate_ack(uint64_t payload);
extern void cap_handle_revoke_req(uint64_t payload, uint32_t source_core);
extern void cap_handle_revoke_ack(uint64_t payload);

static kstatus_t tlb_send_via_transport(uint32_t core, const bharat_monitor_v1_TlbInvalidateReq_t* req, uint64_t reqid) {
    bharat_transport_t* t = transport_for_core(core);
    if (t) {
         return (kstatus_t)bharat_monitor_v1_send_tlb_invalidate(t, core, req, reqid, NULL);
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

static inline uint64_t tlb_get_timeout_ticks(void) {
    uint64_t freq = hal_timer_read_freq();
    if (freq == 0) {
#ifdef BHARAT_HOST_TEST
        return 10000; // Provide an explicit fake monotonic clock timeout in host tests
#else
        kernel_panic("TLB: Monotonic timer is completely unavailable!");
        return 0; // unreachable
#endif
    }
    // 10 ms per attempt
    return freq / 100;
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
        case TLB_FAIL_ISOLATE_ASPACE:
            // Handled in caller using aspace_mark_poisoned
            break;
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

kstatus_t vmm_send_tlb_invalidate_ex(vm_aspace_t *aspace,
                                uint64_t va,
                                uint64_t len,
                                uint32_t type,
                                tlb_failure_policy_t failure_policy)
{
    if (!aspace_is_valid_for_tlb(aspace)) return K_ERR_INVALID_ARG;

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
        tlb_pending_stats_t* stats = tlb_pending_get_stats(current_core);
        if (stats) stats->fallback_count++;
        return K_ERR_NO_RESOURCES;
    }

    bharat_monitor_v1_TlbInvalidateReq_t req = {0};
    req.aspace_id = aspace->object_id;
    req.va_start  = va;
    req.length    = len;
    req.type      = type;
    req.generation = aspace->tlb_gen; // Keep actual coherency generation

    (void)aspace_next_tlb_generation(aspace);

    uint64_t active_target_mask = target_mask;
    uint32_t retry_count = 0;
    kstatus_t status = K_OK;
    uint64_t timeout_ticks = tlb_get_timeout_ticks();
    uint64_t start_tick = hal_timer_monotonic_ticks();

    #define BHARAT_TLB_MAX_RETRIES 3

    while (retry_count < BHARAT_TLB_MAX_RETRIES) {
        if (retry_count > 0) {
            tlb_pending_stats_t* stats = tlb_pending_get_stats(current_core);
            if (stats) stats->retries++;
        }

        bool any_sent = false;
        uint64_t dispatch_failure_mask = 0;

        for (int core = 0; core < MAX_CPUS; core++) {
            if (core == current_core) continue;
            if (!(active_target_mask & (1ULL << core))) continue;

            if (tlb_send_via_transport(core, &req, reqid) == K_OK) {
                 any_sent = true;
            } else {
                 #if BHARAT_TLB_LEGACY_MAILBOX_FALLBACK
                 tlb_send_via_mailbox_legacy(core, current_core, type, aspace, va, len, reqid);
                 any_sent = true;
                 tlb_pending_stats_t* stats = tlb_pending_get_stats(current_core);
                 if (stats) stats->legacy_fallback_usage++;
                 console_log(CONSOLE_LEVEL_WARN, "TLB: Fallback to legacy mailbox on core %u\n", core);
                 #else
                 dispatch_failure_mask |= (1ULL << core);
                 tlb_pending_stats_t* stats = tlb_pending_get_stats(current_core);
                 if (stats) stats->send_failures++;
                 #endif
            }
        }

        if (!any_sent && retry_count == 0) {
            tlb_pending_free(current_core, slot);
            return K_ERR_NOT_FOUND;
        }

        uint64_t attempt_start = hal_timer_monotonic_ticks();
        uint64_t deadline = attempt_start + timeout_ticks;
        bool complete = false;

        while (hal_timer_monotonic_ticks() < deadline) {
            if (tlb_pending_is_complete(current_core, slot)) {
                complete = true;
                break;
            }
            arch_cpu_relax();
            vmm_process_urpc_messages();
        }

        if (complete) {
            status = K_OK;
            break;
        }

        // Timeout or partial completion on this attempt
        uint64_t elapsed_ticks = hal_timer_monotonic_ticks() - start_tick;
        uint64_t missing_mask = tlb_pending_get_missing_mask(current_core, slot);

        tlb_failure_snapshot_t diag = {0};
        diag.request_id = reqid;
        diag.aspace_id = aspace->object_id;
        diag.tlb_generation = req.generation;
        diag.target_mask = target_mask;
        diag.ack_mask = target_mask & ~missing_mask;
        diag.missing_mask = missing_mask;
        diag.dispatch_failure_mask = dispatch_failure_mask;
        diag.retry_count = retry_count;
        diag.start_tick = start_tick;
        diag.elapsed_ticks = elapsed_ticks;
        diag.final_status = (int)K_ERR_TIMEOUT;
        diag.valid = true;

        tlb_diag_set_last_failure(current_core, &diag);

        tlb_pending_stats_t* stats = tlb_pending_get_stats(current_core);
        if (stats) {
            stats->timeouts++;
            if (missing_mask != target_mask) {
                stats->partial_completions++;
            }
        }

        status = K_ERR_TIMEOUT;
        retry_count++;

        if (retry_count < BHARAT_TLB_MAX_RETRIES) {
            // Retry only for the remaining missing CPUs
            active_target_mask = missing_mask;
        }
    }

    if (status != K_OK) {
        tlb_handle_failure(failure_policy, aspace->object_id, reqid);
        if (failure_policy == TLB_FAIL_ISOLATE_ASPACE) {
            aspace_mark_poisoned(aspace);
        }
    }

    tlb_pending_free(current_core, slot);
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

kstatus_t tlb_invalidate_all_ex(vm_aspace_t *aspace, uintptr_t va, size_t len, tlb_inv_kind_t kind, tlb_failure_policy_t failure_policy) {
    if (!aspace || !active_hal_tlb) return K_ERR_INVALID_ARG;

    uint32_t type;
    switch(kind) {
        case TLB_INV_PAGE: type = 0; break;
        case TLB_INV_RANGE: type = 1; break;
        default: type = 2; break;
    }

    kstatus_t status = vmm_send_tlb_invalidate_ex(aspace, va, len, type, failure_policy);
    if (status != K_OK) return status;

    int local_status = tlb_invalidate_local(aspace, va, len, kind);
    if (local_status != 0) return K_ERR_INVALID_ARG;

    return K_OK;
}

int tlb_invalidate_all(vm_aspace_t *aspace, uintptr_t va, size_t len, tlb_inv_kind_t kind) {
    kstatus_t status = tlb_invalidate_all_ex(aspace, va, len, kind, tlb_default_failure_policy());
    return (status == K_OK) ? 0 : -1;
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
                        tx_hdr.src_node      = current_core;
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
                    uint32_t status = 0;
                    if (rx_len >= BHARAT_MSG_HEADER_MIN_LEN + sizeof(bharat_monitor_v1_TlbInvalidateResp_t)) {
                        status = bharat_load_le32(buffer + BHARAT_MSG_HEADER_MIN_LEN);
                    }

                    tlb_pending_ack(req_id, acking_core);

                    if (status != 0) {
                        // Handle NACK status
                    }
                }
            }
        }
    }

    // Process legacy responses (requester-side polling/clearing)
    for (int s = 0; s < BHARAT_TLB_MAX_PENDING_PER_CORE; s++) {
        if (__atomic_load_n(&g_tlb_legacy_resps[current_core][s].valid, __ATOMIC_ACQUIRE)) {
            uint32_t req_id = g_tlb_legacy_resps[current_core][s].request_id;
            uint32_t status = g_tlb_legacy_resps[current_core][s].status;
            uint32_t acking_core = g_tlb_legacy_resps[current_core][s].acking_core;

            // Call tlb_pending_ack locally on the requester core
            tlb_pending_ack(req_id, acking_core);

            // Handle NACK status if status != 0
            if (status != 0) {
                // Potential telemetry or logging of remote execution failure / NACK
            }

            __atomic_store_n(&g_tlb_legacy_resps[current_core][s].valid, 0, __ATOMIC_RELEASE);
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

             // Call safe completion dispatcher to route the ACK/NACK back
             tlb_send_completion(req.generation, mailbox->msg.sender_core, resp.status);
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
