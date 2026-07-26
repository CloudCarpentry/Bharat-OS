#define BHARAT_HOST_TEST 1
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#include "../../kernel/include/hal/hal_tlb.h"
#include "../../kernel/include/mm/aspace.h"
#include "../../kernel/include/mm/mm_remote.h"
#include "../../kernel/include/bharat/cpu_local.h"
#include "../../kernel/include/hal/hal.h"
#include "../../kernel/include/hal/hal_timer.h"
#include "../../kernel/include/mm/tlb.h"
#include "../../kernel/src/mm/tlb/tlb_pending.h"
#include "../../kernel/include/arch/arch_caps.h"
#include <bharat/uapi/subsys/msg_types.h>
#include "../../kernel/include/mm/tlb_internal.h"

// --- Wire structure definitions matching shootdown.c mocks ---
#define BHARAT_MSG_HEADER_MIN_LEN 44
#define BHARAT_MSG_MAGIC         0x42485254
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

// --- Mock Transport System ---
#define MAX_MESSAGES 128
typedef struct {
    uint8_t data[256];
    size_t len;
} mock_msg_t;

typedef struct {
    mock_msg_t queue[MAX_MESSAGES];
    int head;
    int tail;
} mock_queue_t;

static mock_queue_t g_core_inbox[MAX_CPUS];
static bharat_transport_t g_mock_transports[MAX_CPUS];
static bool g_simulate_send_failure = false;

// --- Extern Declarations ---
extern kstatus_t vmm_send_tlb_invalidate_ex(vm_aspace_t *aspace,
                                uint64_t va,
                                uint64_t len,
                                uint32_t type,
                                tlb_failure_policy_t failure_policy);
extern int bharat_monitor_v1_send_tlb_invalidate(bharat_transport_t* t, int dst, const bharat_monitor_v1_TlbInvalidateReq_t* req, uint64_t reqid, void* ctx);

// --- Global Mocks and Globals ---
uint32_t g_active_core_count = 4;
static uint32_t g_current_cpu_id = 0;
uint64_t g_fake_ticks = 0;
static uint64_t g_fake_freq = 1000; // 1 tick = 1 ms

hal_tlb_ops_t *active_hal_tlb;
static hal_tlb_ops_t mock_ops;

mm_mailbox_slot_t g_mm_mailboxes[64];
cpu_local_t g_cpu_locals[32];
tlb_cpu_state_t g_tlb_cpu_state[MAX_CPUS];

static bool g_panic_triggered = false;
static const char* g_panic_message = NULL;

void kernel_panic(const char* m) {
    g_panic_triggered = true;
    g_panic_message = m;
}

uint32_t hal_cpu_get_id(void) {
    return g_current_cpu_id;
}

uint64_t hal_timer_monotonic_ticks(void) {
    return g_fake_ticks;
}

uint64_t hal_timer_read_freq(void) {
    return g_fake_freq;
}

arch_caps_t arch_get_caps(void) {
    arch_caps_t c = { .bits = ARCH_CAP_BIT(ARCH_CAP_SMP) };
    return c;
}

const hal_tlb_caps_t *hal_tlb_caps(void) {
    static hal_tlb_caps_t caps = {
        .supports_page_flush = true,
        .supports_range_flush = true,
        .supports_aspace_flush = true,
        .supports_all_flush = true,
    };
    return &caps;
}

void console_log(int level, const char* fmt, ...) { (void)level; (void)fmt; }
void console_write_raw(const char* s, size_t len) { (void)s; (void)len; }

bool aspace_is_valid_for_tlb(address_space_t *aspace) {
    return aspace != NULL && !(aspace->flags & ASPACE_STATE_POISONED);
}
uint64_t aspace_get_active_mask(address_space_t *aspace) {
    return aspace->active_mask;
}
uint64_t aspace_next_tlb_generation(address_space_t *aspace) {
    return ++aspace->tlb_gen;
}
void aspace_mark_poisoned(address_space_t *aspace) {
    aspace->flags |= ASPACE_STATE_POISONED;
}

// Forward declaration of messages processor helper
static void process_core_messages(uint32_t core_id);

static bool g_cooperative_yield_enabled = true;
static bool g_delayed_ack_test_active = false;
static bool g_duplicate_ack_test_active = false;
static bool g_interleave_test_active = false;

void arch_cpu_relax(void) {
    g_fake_ticks++; // Advance fake clock
    if (g_cooperative_yield_enabled) {
        // Let other cores process messages dynamically
        for (uint32_t i = 0; i < g_active_core_count; i++) {
            if (i != g_current_cpu_id) {
                process_core_messages(i);
            }
        }
    } else {
        if (g_delayed_ack_test_active) {
            if (g_fake_ticks == 5) {
                process_core_messages(1);
            }
        }
        if (g_duplicate_ack_test_active) {
            if (g_fake_ticks == 3) {
                printf("[DEBUG] CPU0 inbox before: head=%d, tail=%d. CPU1 inbox before: head=%d, tail=%d\n",
                       g_core_inbox[0].head, g_core_inbox[0].tail, g_core_inbox[1].head, g_core_inbox[1].tail);
                process_core_messages(1);
                printf("[DEBUG] CPU0 inbox after CPU1 processes: head=%d, tail=%d. CPU1 inbox after: head=%d, tail=%d\n",
                       g_core_inbox[0].head, g_core_inbox[0].tail, g_core_inbox[1].head, g_core_inbox[1].tail);
                mock_queue_t* inbox = &g_core_inbox[0];
                int next_tail = (inbox->tail + 1) % MAX_MESSAGES;
                inbox->queue[inbox->tail] = inbox->queue[inbox->head];
                inbox->tail = next_tail;
                printf("[DEBUG] After duplicate queued: head=%d, tail=%d\n", inbox->head, inbox->tail);
                fflush(NULL);
            }
        }
        if (g_interleave_test_active) {
            if (g_fake_ticks == 3) {
                process_core_messages(1);
                // Insert unrelated IPC frame in the inbox of CPU 0 after the real response
                mock_queue_t* inbox = &g_core_inbox[0];
                int next_tail = (inbox->tail + 1) % MAX_MESSAGES;
                inbox->queue[next_tail].len = 44;
                memset(inbox->queue[next_tail].data, 0, 44);
                bharat_store_le32(inbox->queue[next_tail].data + 0, BHARAT_MSG_MAGIC);
                ((uint8_t*)inbox->queue[next_tail].data)[0x04] = 1;
                bharat_store_le16(inbox->queue[next_tail].data + 0x06, 44);
                bharat_store_le16(inbox->queue[next_tail].data + 0x08, 99); // unrelated
                inbox->tail = next_tail;
            }
        }
    }
}

void hal_tlb_invalidate_local_page(virt_addr_t va) {
    if (active_hal_tlb && active_hal_tlb->flush_page_local) {
        active_hal_tlb->flush_page_local(va);
    }
}

void hal_tlb_invalidate_local_range(virt_addr_t start, size_t len) {
    if (active_hal_tlb && active_hal_tlb->flush_range_local) {
        active_hal_tlb->flush_range_local(start, len);
    }
}

void hal_tlb_invalidate_local_aspace(uint64_t aspace_id) {
    if (active_hal_tlb && active_hal_tlb->flush_asid_local) {
        active_hal_tlb->flush_asid_local(aspace_id & 0xFFFF);
    }
}

int tlb_invalidate_local(vm_aspace_t *as, uintptr_t va, size_t len, tlb_inv_kind_t kind) {
    (void)as; (void)va; (void)len; (void)kind;
    return 0;
}

// Local mock flushes
int g_local_page_flushes = 0;
int g_local_range_flushes = 0;
int g_local_asid_flushes = 0;
int g_local_all_flushes = 0;

static void mock_flush_page_local(virt_addr_t vaddr) { (void)vaddr; g_local_page_flushes++; }
static void mock_flush_range_local(virt_addr_t start, size_t len) { (void)start; (void)len; g_local_range_flushes++; }
static void mock_flush_all_local(void) { g_local_all_flushes++; }
static void mock_flush_asid_local(uint16_t asid) { (void)asid; g_local_asid_flushes++; }

static int mock_transport_send(struct bharat_transport* t, const uint8_t* buf, size_t len) {
    if (g_simulate_send_failure) return -1;

    bharat_msg_header_t hdr;
    // Decode with mock decode to inspect destination
    hdr.version_major = buf[0x04];
    hdr.version_minor = buf[0x05];
    hdr.header_len    = bharat_load_le16(buf + 0x06);
    hdr.dst_node      = bharat_load_le32(buf + 0x20);

    uint32_t dst = hdr.dst_node;
    if (dst >= MAX_CPUS) return -1;

    mock_queue_t* inbox = &g_core_inbox[dst];
    int next_tail = (inbox->tail + 1) % MAX_MESSAGES;
    if (next_tail == inbox->head) return -1; // Full

    memcpy(inbox->queue[inbox->tail].data, buf, len);
    inbox->queue[inbox->tail].len = len;
    inbox->tail = next_tail;
    return 0;
}

static int mock_transport_recv(struct bharat_transport* t, uint8_t* buf, size_t max, size_t* rx_len) {
    uint32_t current_core = g_current_cpu_id;
    mock_queue_t* inbox = &g_core_inbox[current_core];
    if (inbox->head == inbox->tail) return -1; // Empty

    mock_msg_t* m = &inbox->queue[inbox->head];
    if (m->len > max) return -1;

    memcpy(buf, m->data, m->len);
    *rx_len = m->len;
    inbox->head = (inbox->head + 1) % MAX_MESSAGES;
    return 0;
}

bharat_transport_t* transport_for_core(int core) {
    if (core >= (int)g_active_core_count) return NULL;
    return &g_mock_transports[core];
}

// Unused bootstrap/IPI mocks
void hal_ipi_send(uint32_t core, uint32_t ipi) { (void)core; (void)ipi; }
int urpc_bootstrap_recv(int c, uint64_t *m) { (void)c; (void)m; return -1; }
void cap_handle_delegate_req(uint64_t p, uint32_t s) { (void)p; (void)s; }
void cap_handle_delegate_ack(uint64_t p) { (void)p; }
void cap_handle_revoke_req(uint64_t p, uint32_t s) { (void)p; (void)s; }
void cap_handle_revoke_ack(uint64_t p) { (void)p; }

// --- Test Setup ---
static void setup_test(void) {
    g_current_cpu_id = 0;
    g_fake_ticks = 0;
    g_fake_freq = 1000;
    g_simulate_send_failure = false;
    g_panic_triggered = false;
    g_panic_message = NULL;
    g_cooperative_yield_enabled = true;
    g_delayed_ack_test_active = false;
    g_duplicate_ack_test_active = false;
    g_interleave_test_active = false;

    active_hal_tlb = &mock_ops;
    mock_ops.flush_page_local = mock_flush_page_local;
    mock_ops.flush_range_local = mock_flush_range_local;
    mock_ops.flush_all_local = mock_flush_all_local;
    mock_ops.flush_asid_local = mock_flush_asid_local;

    g_local_page_flushes = 0;
    g_local_range_flushes = 0;
    g_local_asid_flushes = 0;
    g_local_all_flushes = 0;

    memset(g_cpu_locals, 0, sizeof(g_cpu_locals));
    memset(g_mm_mailboxes, 0, sizeof(g_mm_mailboxes));
    memset(g_core_inbox, 0, sizeof(g_core_inbox));

    static struct {
        int (*send)(struct bharat_transport* self, const uint8_t* buf, size_t len);
        int (*recv)(struct bharat_transport* self, uint8_t* buf, size_t cap, size_t* out_len);
        uint32_t (*get_caps)(struct bharat_transport* self);
        size_t (*get_mtu)(struct bharat_transport* self);
        int (*poll)(struct bharat_transport* self, int timeout_ms);
    } ops = {
        .send = mock_transport_send,
        .recv = mock_transport_recv,
        .get_caps = NULL,
        .get_mtu = NULL,
        .poll = NULL
    };

    for (int i = 0; i < MAX_CPUS; i++) {
        g_mock_transports[i].ops = (void*)&ops;
    }

    tlb_pending_init();
}

// Helper to let a specific core process its inbox
static void process_core_messages(uint32_t core_id) {
    uint32_t saved = g_current_cpu_id;
    g_current_cpu_id = core_id;
    extern void vmm_process_urpc_messages(void);
    vmm_process_urpc_messages();
    g_current_cpu_id = saved;
}

// --- Test Cases ---

void test_single_remote_ack(void) {
    printf("Running test_single_remote_ack...\n");
    setup_test();

    address_space_t as = { .object_id = 42, .active_mask = (1ULL << 0) | (1ULL << 1), .tlb_gen = 100 };
    g_cpu_locals[0].current_as_id = 42;
    g_cpu_locals[1].current_as_id = 42;

    // CPU 0 initiates shootdown
    g_current_cpu_id = 0;
    kstatus_t status = vmm_send_tlb_invalidate_ex(&as, 0x1000, 0x1000, 0, TLB_FAIL_RETURN_ERROR);

    assert(status == K_OK);
    printf("  -> PASSED\n");
}

void test_non_zero_initiator(void) {
    printf("Running test_non_zero_initiator...\n");
    setup_test();

    address_space_t as = { .object_id = 42, .active_mask = (1ULL << 2) | (1ULL << 3), .tlb_gen = 100 };
    g_cpu_locals[2].current_as_id = 42;
    g_cpu_locals[3].current_as_id = 42;

    // CPU 2 initiates shootdown
    g_current_cpu_id = 2;
    kstatus_t status = vmm_send_tlb_invalidate_ex(&as, 0x1000, 0x1000, 0, TLB_FAIL_RETURN_ERROR);

    assert(status == K_OK);
    printf("  -> PASSED\n");
}

void test_page_range_aspace_invalidation_dispatch(void) {
    printf("Running test_page_range_aspace_invalidation_dispatch...\n");
    setup_test();

    address_space_t as = { .object_id = 42, .active_mask = (1ULL << 0) | (1ULL << 1), .tlb_gen = 100 };
    g_cpu_locals[0].current_as_id = 42;
    g_cpu_locals[1].current_as_id = 42;

    // Page Invalidation (type=0)
    g_current_cpu_id = 0;
    vmm_send_tlb_invalidate_ex(&as, 0x1000, 4096, 0, TLB_FAIL_RETURN_ERROR);
    assert(g_local_page_flushes == 1);

    // Range Invalidation (type=1)
    vmm_send_tlb_invalidate_ex(&as, 0x2000, 8192, 1, TLB_FAIL_RETURN_ERROR);
    assert(g_local_range_flushes == 1);

    // Aspace Invalidation (type=2)
    vmm_send_tlb_invalidate_ex(&as, 0, 0, 2, TLB_FAIL_RETURN_ERROR);
    assert(g_local_asid_flushes == 1);

    printf("  -> PASSED\n");
}

void test_delayed_ack(void) {
    printf("Running test_delayed_ack...\n");
    setup_test();

    address_space_t as = { .object_id = 42, .active_mask = (1ULL << 0) | (1ULL << 1), .tlb_gen = 100 };
    g_cpu_locals[0].current_as_id = 42;
    g_cpu_locals[1].current_as_id = 42;

    g_current_cpu_id = 0;
    g_cooperative_yield_enabled = false;
    g_delayed_ack_test_active = true;

    kstatus_t status = vmm_send_tlb_invalidate_ex(&as, 0x1000, 4096, 0, TLB_FAIL_RETURN_ERROR);
    assert(status == K_OK);
    printf("  -> PASSED\n");
}

void test_duplicate_ack(void) {
    printf("Running test_duplicate_ack...\n");
    setup_test();

    address_space_t as = { .object_id = 42, .active_mask = (1ULL << 0) | (1ULL << 1), .tlb_gen = 100 };
    g_cpu_locals[0].current_as_id = 42;
    g_cpu_locals[1].current_as_id = 42;

    g_current_cpu_id = 0;
    g_cooperative_yield_enabled = false;
    g_duplicate_ack_test_active = true;

    tlb_pending_stats_t* stats = tlb_pending_get_stats(0);
    uint64_t initial_dups = stats->duplicate_acks;

    kstatus_t status = vmm_send_tlb_invalidate_ex(&as, 0x1000, 4096, 0, TLB_FAIL_RETURN_ERROR);
    assert(status == K_OK);

    assert(stats->duplicate_acks == initial_dups + 1);

    printf("  -> PASSED\n");
}

void test_stale_generation_ack(void) {
    printf("Running test_stale_generation_ack...\n");
    setup_test();

    address_space_t as = { .object_id = 42, .active_mask = (1ULL << 0) | (1ULL << 1), .tlb_gen = 100 };
    g_cpu_locals[0].current_as_id = 42;
    g_cpu_locals[1].current_as_id = 42;

    g_current_cpu_id = 0;

    // Allocate transaction manually to get a req_id with an old generation/sequence
    uint32_t reqid_old = 0;
    int slot = tlb_pending_alloc(42, 1ULL << 1, &reqid_old);
    tlb_pending_free(0, slot);

    // Now start a new transaction
    uint32_t reqid_new = 0;
    slot = tlb_pending_alloc(42, 1ULL << 1, &reqid_new);

    // Acknowledge the old req_id on CPU 0
    tlb_pending_ack(reqid_old, 1);

    // Transaction should NOT be complete!
    assert(!tlb_pending_is_complete(0, slot));

    tlb_pending_free(0, slot);
    printf("  -> PASSED\n");
}

void test_interleaving_unrelated_ipc(void) {
    printf("Running test_interleaving_unrelated_ipc...\n");
    setup_test();

    address_space_t as = { .object_id = 42, .active_mask = (1ULL << 0) | (1ULL << 1), .tlb_gen = 100 };
    g_cpu_locals[0].current_as_id = 42;
    g_cpu_locals[1].current_as_id = 42;

    g_current_cpu_id = 0;
    g_cooperative_yield_enabled = false;
    g_interleave_test_active = true;

    tlb_pending_stats_t* stats = tlb_pending_get_stats(0);
    uint64_t initial_acks = stats->acks_received;

    kstatus_t status = vmm_send_tlb_invalidate_ex(&as, 0x1000, 4096, 0, TLB_FAIL_RETURN_ERROR);
    assert(status == K_OK);

    assert(stats->acks_received == initial_acks + 1);
    printf("  -> PASSED\n");
}

void test_one_target_missing_timeout_retry(void) {
    printf("Running test_one_target_missing_timeout_retry...\n");
    setup_test();

    address_space_t as = { .object_id = 42, .active_mask = (1ULL << 0) | (1ULL << 1) | (1ULL << 2), .tlb_gen = 100 };
    g_cpu_locals[0].current_as_id = 42;
    g_cpu_locals[1].current_as_id = 42;
    g_cpu_locals[2].current_as_id = 42;

    g_current_cpu_id = 0;
    g_cooperative_yield_enabled = false;

    // First attempt
    uint32_t reqid = 0;
    int slot = tlb_pending_alloc(42, (1ULL << 1) | (1ULL << 2), &reqid);

    // We send requests manually to CPU 1 and 2 using the public async function
    bharat_monitor_v1_TlbInvalidateReq_t req = { .aspace_id = 42, .va_start = 0x1000, .length = 4096, .type = 0, .generation = 100 };
    bharat_monitor_v1_send_tlb_invalidate(transport_for_core(1), 1, &req, reqid, NULL);
    bharat_monitor_v1_send_tlb_invalidate(transport_for_core(2), 2, &req, reqid, NULL);

    // CPU 1 processes and ACKs, CPU 2 is missing
    process_core_messages(1);
    process_core_messages(0);

    // Let the first attempt timeout by advancing tick count by 11 ms
    g_fake_ticks += 11;

    // Get missing mask
    uint64_t missing = tlb_pending_get_missing_mask(0, slot);
    assert(missing == (1ULL << 2)); // 0b0100 (Only CPU 2 was missing)

    tlb_pending_free(0, slot);
    printf("  -> PASSED\n");
}

void test_failure_policies(void) {
    printf("Running test_failure_policies...\n");

    setup_test();
    address_space_t as = { .object_id = 42, .active_mask = (1ULL << 0) | (1ULL << 1), .tlb_gen = 100 };
    g_cpu_locals[0].current_as_id = 42;
    g_cpu_locals[1].current_as_id = 42;
    g_current_cpu_id = 0;

    // Policy: RETURN_ERROR
    g_cooperative_yield_enabled = false; // Freeze other cores to force timeout!
    kstatus_t status = vmm_send_tlb_invalidate_ex(&as, 0x1000, 4096, 0, TLB_FAIL_RETURN_ERROR);
    assert(status == K_ERR_TIMEOUT);
    assert(!g_panic_triggered);

    // Policy: ISOLATE_ASPACE
    setup_test();
    g_cooperative_yield_enabled = false;
    status = vmm_send_tlb_invalidate_ex(&as, 0x1000, 4096, 0, TLB_FAIL_ISOLATE_ASPACE);
    assert(status == K_ERR_TIMEOUT);
    assert(as.flags & ASPACE_STATE_POISONED); // ASpace marked poisoned!

    // Verify isolation and mutation rejection end-to-end!
    // Since aspace is poisoned/isolated, subsequent invalidation/mutation checks should fail closed
    kstatus_t bad_status = vmm_send_tlb_invalidate_ex(&as, 0x1000, 4096, 0, TLB_FAIL_RETURN_ERROR);
    assert(bad_status == K_ERR_INVALID_ARG);

    // Policy: KERNEL_PANIC
    setup_test();
    as.flags = 0; // Reset poisoned flag so it enters wait loop
    g_cooperative_yield_enabled = false;
    status = vmm_send_tlb_invalidate_ex(&as, 0x1000, 4096, 0, TLB_FAIL_KERNEL_PANIC);
    assert(g_panic_triggered);

    printf("  -> PASSED\n");
}

int main(void) {
    printf("=========================================\n");
    printf("Running Comprehensive TLB Transport Host Tests\n");
    printf("=========================================\n");

    test_single_remote_ack();
    test_non_zero_initiator();
    test_page_range_aspace_invalidation_dispatch();
    test_delayed_ack();
    test_duplicate_ack();
    test_stale_generation_ack();
    test_interleaving_unrelated_ipc();
    test_one_target_missing_timeout_retry();
    test_failure_policies();

    printf("\nAll KERN-P0-002 v2 host tests PASSED successfully!\n");
    return 0;
}
