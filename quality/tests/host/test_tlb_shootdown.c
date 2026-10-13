#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>

#include <hal/hal_tlb.h>
#include <mm/aspace.h>
#include <mm/mm_remote.h>
#include <bharat/cpu_local.h>
#include <hal/hal.h>
#include <urpc/urpc_bootstrap.h>
#include <arch/arch_caps.h>
#include <mm/tlb_internal.h>
#include <kernel/status.h>

typedef struct {
    uint64_t aspace_id;
    uint64_t va_start;
    uint64_t length;
    uint32_t type;
    uint32_t generation;
} bharat_monitor_v1_TlbInvalidateReq_t;

typedef void* bharat_transport_t;

// --- Mocking ---

hal_tlb_ops_t *active_hal_tlb;
static hal_tlb_ops_t mock_ops;

int g_local_page_flushes = 0;
int g_local_range_flushes = 0;
int g_local_asid_flushes = 0;
int g_local_all_flushes = 0;

static void mock_flush_page_local(virt_addr_t vaddr) { (void)vaddr; g_local_page_flushes++; }
static void mock_flush_range_local(virt_addr_t start, size_t len) { (void)start; (void)len; g_local_range_flushes++; }
static void mock_flush_all_local(void) { g_local_all_flushes++; }
static void mock_flush_asid_local(uint16_t asid) { (void)asid; g_local_asid_flushes++; }

static uint32_t g_current_cpu_id = 0;

uint32_t hal_cpu_get_id(void) {
    return g_current_cpu_id;
}

void hal_core_poll_event(void) {
    // Just yield/relax
    __asm__ volatile("rep nop" ::: "memory");
}

int urpc_is_ready(uint32_t core) {
    return 1; // All cores ready
}

int urpc_bootstrap_send(uint32_t core, uint64_t msg) {
    (void)core; (void)msg;
    return 0;
}

uint32_t g_active_core_count = 4;
tlb_cpu_state_t g_tlb_cpu_state[32];
cpu_local_t g_cpu_locals[32];

mem_model_t mem_model_get_current(void) { return MEM_MODEL_MMU_FULL; }

int bharat_monitor_v1_send_tlb_invalidate(bharat_transport_t* t, int dst, const bharat_monitor_v1_TlbInvalidateReq_t* req, uint64_t reqid, void* ctx) {
    (void)t; (void)dst; (void)req; (void)reqid; (void)ctx;
    return 0;
}

static uint64_t g_fake_ticks = 0;
bool hal_timer_monotonic_ns(uint64_t *out_ns) {
    if (out_ns) *out_ns = g_fake_ticks * 1000000ULL;
    return true;
}

void arch_cpu_relax(void) {
    g_fake_ticks++;
}

arch_caps_t arch_get_caps(void) {
    arch_caps_t c = {ARCH_CAP_BIT(ARCH_CAP_SMP)};
    return c;
}

const hal_tlb_caps_t *hal_tlb_caps(void) {
    static hal_tlb_caps_t c = {
        .supports_page_flush = 1,
        .supports_range_flush = 1,
        .supports_aspace_flush = 1,
        .supports_all_flush = 1
    };
    return &c;
}

void console_log(int level, const char* fmt, ...) { (void)level; (void)fmt; }
void console_write_raw(const char* s, size_t len) { (void)s; (void)len; }
void kernel_panic(const char* m) { (void)m; }

bool aspace_is_valid_for_tlb(address_space_t *aspace) { return aspace != NULL; }
uint64_t aspace_get_active_mask(address_space_t *aspace) { return aspace ? aspace->active_mask : 0; }
uint64_t aspace_next_tlb_generation(address_space_t *aspace) { return aspace ? ++aspace->tlb_gen : 0; }
void aspace_mark_poisoned(address_space_t *aspace) { if (aspace) aspace->flags |= ASPACE_STATE_POISONED; }

bharat_transport_t* transport_for_core(int core) { (void)core; return NULL; }

void hal_ipi_send(uint32_t target_core, uint32_t ipi) {
    (void)ipi;
    if (target_core < 64) {
        mm_mailbox_slot_t* mailbox = &g_mm_mailboxes[target_core];
        if (mailbox->valid) {
            mailbox->ack_seq = mailbox->req_seq;
            mailbox->valid = 0;
            extern void tlb_send_completion(uint32_t request_id, uint32_t origin_cpu, uint32_t status);
            uint32_t saved_cpu = g_current_cpu_id;
            g_current_cpu_id = target_core;
            tlb_send_completion(mailbox->msg.seq, mailbox->msg.sender_core, 0);
            g_current_cpu_id = saved_cpu;
        }
    }
}

void cap_handle_delegate_req(uint64_t p, uint32_t s) { (void)p; (void)s; }
void cap_handle_delegate_ack(uint64_t p) { (void)p; }
void cap_handle_revoke_req(uint64_t p, uint32_t s) { (void)p; (void)s; }
void cap_handle_revoke_ack(uint64_t p) { (void)p; }
kstatus_t urpc_bootstrap_recv(uint32_t c, uint64_t *m) { (void)c; (void)m; return -1; }

// Ensure mailboxes are defined
mm_mailbox_slot_t g_mm_mailboxes[64];

static void setup_test() {
    g_current_cpu_id = 0;
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
    for (int i=0; i<64; i++) {
        spin_lock_init(&g_mm_mailboxes[i].lock);
    }
}

void test_tlb_shootdown_page_local_only() {
    setup_test();
    address_space_t as = {0};
    as.object_id = 1;
    as.tlb_gen = 0;

    // Core 0 is running AS 1
    g_cpu_locals[0].current_as = &as;
    g_cpu_locals[0].current_as_id = 1;
    as.active_mask = 1ULL << 0;

    hal_tlb_invalidate_page(&as, 0x1000);

    assert(g_local_page_flushes == 1);
    assert(as.tlb_gen == 1);

    // Remote cores shouldn't have received anything (ack_seq 0)
    assert(g_mm_mailboxes[1].ack_seq == 0);
    printf("test_tlb_shootdown_page_local_only PASSED\n");
}

void test_tlb_shootdown_page_remote() {
    setup_test();
    address_space_t as = {0};
    as.object_id = 2;
    as.tlb_gen = 0;

    // Core 0 (initiator) is running AS 1
    g_cpu_locals[0].current_as_id = 1;

    // Core 1 (remote) is running AS 2
    g_cpu_locals[1].current_as = &as;
    g_cpu_locals[1].current_as_id = 2;
    as.active_mask = 1ULL << 1;

    // Core 2 is running AS 3 (should not receive shootdown)
    g_cpu_locals[2].current_as_id = 3;

    hal_tlb_invalidate_page(&as, 0x2000);

    // Initiator is NOT running AS 2, so no local flush
    assert(g_local_page_flushes == 0);
    assert(as.tlb_gen == 1);

    // Core 1 should have received a request, and auto-acked via mock IPI
    assert(g_mm_mailboxes[1].req_seq == 1);
    assert(g_mm_mailboxes[1].ack_seq == 1);
    assert(g_mm_mailboxes[1].msg.scope == TLB_SCOPE_PAGE);

    // Core 2 should NOT have received a request
    assert(g_mm_mailboxes[2].req_seq == 0);

    printf("test_tlb_shootdown_page_remote PASSED\n");
}

void test_tlb_shootdown_range_and_aspace() {
    setup_test();
    address_space_t as = {0};
    as.object_id = 3;
    as.active_mask = 1ULL << 0;

    g_cpu_locals[0].current_as = &as;
    g_cpu_locals[0].current_as_id = 3;

    hal_tlb_invalidate_range(&as, 0x4000, 8192);
    assert(g_local_range_flushes == 1);
    assert(as.tlb_gen == 1);

    hal_tlb_invalidate_aspace(&as);
    assert(g_local_asid_flushes == 1);
    assert(as.tlb_gen == 2);

    printf("test_tlb_shootdown_range_and_aspace PASSED\n");
}

int main() {
    test_tlb_shootdown_page_local_only();
    test_tlb_shootdown_page_remote();
    test_tlb_shootdown_range_and_aspace();
    printf("All TLB shootdown tests passed.\n");
    return 0;
}
