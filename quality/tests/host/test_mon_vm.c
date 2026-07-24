#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "../../kernel/include/monitor/mon_vm_ops.h"
#include "../../kernel/src/monitor/mon_vm_state.h"
#include "../../kernel/include/hal/hal.h"

// --- Mocking ---

uint32_t hal_cpu_get_id(void) {
    return 1; // Simulate we are core 1 handling an ACK or sending
}

int urpc_is_ready(uint32_t core) {
    return 1;
}

int urpc_bootstrap_core(uint32_t core_id) {
    return 0;
}

int urpc_bootstrap_send(uint32_t target_core, uint64_t msg) {
    return 0;
}

void hal_core_poll_event(void) {
    // Simulate receiving an ACK on the next poll if we have pending transactions
    // For test simplicity, we auto-ACK the first active transaction from core 2
    spinlock_acquire(&g_mon_vm_state.lock);
    for (int i = 0; i < MAX_PENDING_TXNS; i++) {
        if (g_mon_vm_state.pending_txns[i].in_use && g_mon_vm_state.pending_txns[i].final_status == MON_VM_STATUS_SUCCESS) {
            mon_vm_pending_txn_t *txn = &g_mon_vm_state.pending_txns[i];

            // Only auto-ack if we haven't already
            if (txn->expected_acks_mask != 0 && (txn->received_acks_mask & txn->expected_acks_mask) != txn->expected_acks_mask) {
                // Simulate an incoming ACK message
                mon_vm_ack_msg_t ack = {0};
                ack.h.type = MON_VM_ACK;
                ack.h.txn_id = txn->txn_id;
                ack.h.src_core = 2; // Assuming we wait for core 2
                ack.status = MON_VM_STATUS_SUCCESS;

                spinlock_release(&g_mon_vm_state.lock);

                mon_vm_dispatch(&ack, sizeof(ack));

                return;
            }
        }
    }
    spinlock_release(&g_mon_vm_state.lock);
}

static void reset_state() {
    memset(&g_mon_vm_state, 0, sizeof(g_mon_vm_state));
    mon_vm_init();
}

void test_initialization() {
    reset_state();
    assert(g_mon_vm_state.initialized == true);
    assert(g_mon_vm_state.next_txn_id == 1);
    printf("test_initialization PASSED\n");
}

void test_dispatch_malformed() {
    reset_state();
    int res = mon_vm_dispatch(NULL, 100);
    assert(res == MON_VM_STATUS_MALFORMED);

    char small_buf[4] = {0};
    res = mon_vm_dispatch(small_buf, 4);
    assert(res == MON_VM_STATUS_MALFORMED);

    // Map with unaligned addresses
    mon_vm_map_msg_t msg = {0};
    msg.h.type = MON_VM_MAP;
    msg.va_start = 0x1005; // Unaligned
    msg.length = 4096;
    res = mon_vm_dispatch(&msg, sizeof(msg));
    assert(res == MON_VM_STATUS_MALFORMED);

    printf("test_dispatch_malformed PASSED\n");
}

void test_send_map_strict() {
    reset_state();

    vm_space_t space = {0};
    space.space_id = 1;
    space.generation = 1;
    space.active_cores = (1ULL << 2); // Core 2

    vm_map_req_t req = {0};
    req.va = 0x1000;
    req.pa = 0x2000;
    req.len = 4096;

    int res = mon_vm_send_map(&space, &req, true);

    assert(res == MON_VM_STATUS_SUCCESS);
    assert(g_mon_vm_state.stat_requests_sent == 1);
    assert(g_mon_vm_state.stat_acks_received == 1); // Mock auto-acked

    printf("test_send_map_strict PASSED\n");
}

void test_unsupported_ops() {
    reset_state();

    mon_vm_hdr_t msg = {0};
    msg.type = MON_VM_REALIZE;

    int res = mon_vm_dispatch(&msg, sizeof(mon_vm_hdr_t)); // Using hdr size as minimum
    assert(res == MON_VM_STATUS_UNSUPPORTED);

    printf("test_unsupported_ops PASSED\n");
}

int main() {
    test_initialization();
    test_dispatch_malformed();
    test_send_map_strict();
    test_unsupported_ops();
    printf("All Monitor VM tests passed.\n");
    return 0;
}
