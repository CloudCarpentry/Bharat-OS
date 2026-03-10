#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "ipc_endpoint.h"
#include "capability.h"
#include "sched.h"
#include <stdlib.h>
#include "../kernel/include/slab.h"

// Mock dependencies
void ipc_async_check_timeouts(uint64_t current_ticks) { (void)current_ticks; }
static address_space_t g_as = { .root_table = 0x1000U };
address_space_t* mm_create_address_space(void) { return &g_as; }
phys_addr_t mm_alloc_page(uint32_t preferred_numa_node) { (void)preferred_numa_node; return 0; }
void mm_free_page(phys_addr_t page) { (void)page; }
phys_addr_t pmm_alloc_pages_colored(int order, uint32_t preferred_numa_node, uint32_t flags, mm_color_config_t *color_config) { return 0; }
phys_addr_t mm_alloc_pages_order(int order, uint32_t preferred_numa_node, uint32_t flags) { return 0; }
void tlb_shootdown(virt_addr_t vaddr) { (void)vaddr; }


static void dummy_entry(void) {}

void test_ipc_sync_sender_wakeup(void) {
    sched_init();

    kprocess_t* proc = process_create("test_sender");
    assert(proc != NULL);

    capability_table_t* table = (capability_table_t*)proc->security_sandbox_ctx;
    assert(table != NULL);

    uint32_t send_cap, recv_cap;
    assert(ipc_endpoint_create(table, &send_cap, &recv_cap) == IPC_OK);

    kthread_t* t_sender1 = thread_create(proc, dummy_entry);
    kthread_t* t_sender2 = thread_create(proc, dummy_entry);
    kthread_t* t_receiver = thread_create(proc, dummy_entry);

    // Initial state: sender1 fills the endpoint buffer.
    // To pretend t_sender1 is running:
    while (sched_current_thread() != t_sender1) {
        sched_yield();
    }
    assert(sched_current_thread() == t_sender1);

    const char* msg1 = "msg1";
    assert(ipc_endpoint_send(table, send_cap, msg1, 5) == IPC_OK);

    // Switch to sender2. Attempting to send should block it.
    while (sched_current_thread() != t_sender2) {
        sched_yield();
    }
    assert(sched_current_thread() == t_sender2);

    const char* msg2 = "msg2";
    assert(ipc_endpoint_send(table, send_cap, msg2, 5) == IPC_ERR_WOULD_BLOCK);
    assert(t_sender2->state == THREAD_STATE_BLOCKED);

    // Switch to receiver. Receiver receives first message.
    // Since sender2 is blocked, it won't be scheduled, we must run yield until receiver is active.
    while (sched_current_thread() != t_receiver) {
        sched_yield();
    }
    assert(sched_current_thread() == t_receiver);

    char buf[16];
    uint32_t len = 0;
    assert(ipc_endpoint_receive(table, recv_cap, buf, sizeof(buf), &len) == IPC_OK);
    assert(len == 5);
    assert(strcmp(buf, "msg1") == 0);

    // Now t_sender2 should be READY.
    assert(t_sender2->state == THREAD_STATE_READY);

    printf("test_ipc_sync_sender_wakeup passed\n");
}

int main(void) {
    test_ipc_sync_sender_wakeup();
    return 0;
}
