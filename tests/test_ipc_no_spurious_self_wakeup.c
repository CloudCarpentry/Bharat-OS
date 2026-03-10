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

void test_ipc_no_spurious_self_wakeup(void) {
    sched_init();

    kprocess_t* proc = process_create("test_spurious");
    assert(proc != NULL);

    capability_table_t* table = (capability_table_t*)proc->security_sandbox_ctx;
    assert(table != NULL);

    uint32_t send_cap, recv_cap;
    assert(ipc_endpoint_create(table, &send_cap, &recv_cap) == IPC_OK);

    kthread_t* self = thread_create(proc, dummy_entry);

    while (sched_current_thread() != self) {
        sched_yield();
    }
    assert(sched_current_thread() == self);

    // Initial state: buffer is empty. We attempt to send to ourselves
    // Buffer has size 1, so the first send works.
    assert(ipc_endpoint_send(table, send_cap, "msg", 4) == IPC_OK);

    // Now buffer is full. If we attempt to send again, we should block,
    // but the bug previously was waking up "current thread", so let's verify
    // that our state genuinely stays blocked, and no subsequent operation spuriously wakes us.
    assert(ipc_endpoint_send(table, send_cap, "msg2", 5) == IPC_ERR_WOULD_BLOCK);
    assert(self->state == THREAD_STATE_BLOCKED);

    // The thread is genuinely blocked and wouldn't be able to run.

    printf("test_ipc_no_spurious_self_wakeup passed\n");
}

int main(void) {
    test_ipc_no_spurious_self_wakeup();
    return 0;
}
