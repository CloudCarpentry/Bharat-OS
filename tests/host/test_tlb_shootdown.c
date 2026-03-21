#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>

#include "../../kernel/include/hal/hal_tlb.h"
#include "../../kernel/include/mm/aspace.h"
#include "../../kernel/include/mm/mm_remote.h"
#include "../../kernel/include/bharat/cpu_local.h"
#include "../../kernel/include/hal/hal.h"
#include "../../kernel/include/urpc/urpc_bootstrap.h"
#include "../../subsys/include/bharat/msg/transport.h"
#include "../../services/monitor/generated/bharat_monitor_v1_types.h"

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

uint32_t hal_cpu_get_id(void) {
    return 0; // Simulate we are always core 0 for the initiator
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

void hal_send_ipi_payload(uint32_t target_core, uint64_t payload) {
    (void)target_core; (void)payload;
    // Mock IPI: Auto-ACK for testing purposes by simulating remote core
    mm_mailbox_slot_t* mailbox = &g_mm_mailboxes[target_core];
    if (mailbox->valid) {
        mailbox->ack_seq = mailbox->msg.seq;
        mailbox->valid = 0;
    }
}

int bharat_monitor_v1_call_tlb_invalidate(
    bharat_transport_t* t, int dst, const bharat_monitor_v1_TlbInvalidateReq_t* req, void* ctx) {
    return 0;
}

// Ensure mailboxes are defined
mm_mailbox_slot_t g_mm_mailboxes[64];

static void setup_test() {
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
