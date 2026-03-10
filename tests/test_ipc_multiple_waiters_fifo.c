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

void test_ipc_multiple_waiters_fifo(void) {
    sched_init();

    kprocess_t* proc = process_create("test_fifo");
    assert(proc != NULL);

    capability_table_t* table = (capability_table_t*)proc->security_sandbox_ctx;
    assert(table != NULL);

    uint32_t send_cap, recv_cap;
    assert(ipc_endpoint_create(table, &send_cap, &recv_cap) == IPC_OK);

    kthread_t* r1 = thread_create(proc, dummy_entry);
    kthread_t* r2 = thread_create(proc, dummy_entry);
    kthread_t* r3 = thread_create(proc, dummy_entry);
    kthread_t* sender = thread_create(proc, dummy_entry);

    // Get all receivers blocked
    char buf[16];
    uint32_t len;

    while (sched_current_thread() != r1) { sched_yield(); }
    assert(ipc_endpoint_receive(table, recv_cap, buf, sizeof(buf), &len) == IPC_ERR_WOULD_BLOCK);
    assert(r1->state == THREAD_STATE_BLOCKED);

    while (sched_current_thread() != r2) { sched_yield(); }
    assert(ipc_endpoint_receive(table, recv_cap, buf, sizeof(buf), &len) == IPC_ERR_WOULD_BLOCK);
    assert(r2->state == THREAD_STATE_BLOCKED);

    while (sched_current_thread() != r3) { sched_yield(); }
    assert(ipc_endpoint_receive(table, recv_cap, buf, sizeof(buf), &len) == IPC_ERR_WOULD_BLOCK);
    assert(r3->state == THREAD_STATE_BLOCKED);

    // Switch to sender, and send one message
    while (sched_current_thread() != sender) { sched_yield(); }
    assert(ipc_endpoint_send(table, send_cap, "msg", 4) == IPC_OK);

    // Check that exactly the FIRST blocked thread (r1) was woken
    assert(r1->state == THREAD_STATE_READY);
    assert(r2->state == THREAD_STATE_BLOCKED);
    assert(r3->state == THREAD_STATE_BLOCKED);

    // Wait, the buffer is full now because the receiver was only woken up and didn't consume the message yet.
    // If we send again, it should return IPC_ERR_WOULD_BLOCK since buffer size is 1.
    // So let's consume the message with r1 first, which should be able to run and empty the buffer.
    while (sched_current_thread() != r1) { sched_yield(); }
    assert(ipc_endpoint_receive(table, recv_cap, buf, sizeof(buf), &len) == IPC_OK);

    // Now send the second message
    while (sched_current_thread() != sender) { sched_yield(); }
    assert(ipc_endpoint_send(table, send_cap, "msg", 4) == IPC_OK);
    assert(r2->state == THREAD_STATE_READY);
    assert(r3->state == THREAD_STATE_BLOCKED);

    printf("test_ipc_multiple_waiters_fifo passed\n");
}

int main(void) {
    test_ipc_multiple_waiters_fifo();
    return 0;
}
