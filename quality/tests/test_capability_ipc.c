#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../kernel/include/capability.h"
#include "../kernel/include/ipc_endpoint.h"

void ipc_async_check_timeouts(uint64_t current_ticks) {
    (void)current_ticks;
}

static address_space_t g_as = { .root_pt = 0x1000U };

address_space_t* mm_create_address_space(void) {
    return &g_as;
}

phys_addr_t mm_alloc_page(uint32_t preferred_numa_node) {
    (void)preferred_numa_node;
    return 0;
}

void mm_free_page(phys_addr_t page) {
    (void)page;
}





void entry_point(void) {}

int main(void) {
    sched_init();

    bh_process_t* proc = process_create("ipc");
    assert(proc != NULL);

    cap_table_init_for_process(proc);

    capability_table_t* table = (capability_table_t*)proc->security_sandbox_ctx;
    assert(table != NULL);

    uint32_t send_cap = 0;
    uint32_t recv_cap = 0;
    assert(ipc_endpoint_create(table, &send_cap, &recv_cap) == IPC_OK);

    const char* msg = "hello";
    assert(ipc_endpoint_send(table, send_cap, msg, 5U, 0, 0, 0) == IPC_OK);

    char out[16] = {0};
    uint32_t out_len = 0;
    assert(ipc_endpoint_receive(table, recv_cap, out, sizeof(out), &out_len, 0, NULL) == IPC_OK);
    assert(out_len == 5U);
    assert(out[0] == 'h' && out[4] == 'o');

    // Delegation must only narrow rights.
    uint32_t delegated = 0;
    assert(cap_table_delegate(table, table, send_cap, CAP_RIGHT_ENDPOINT_SEND, &delegated) == 0);

    // ==========================================
    // Production-Grade Capability Transfer Tests
    // ==========================================

    // 1. Transfer of a valid transferable cap succeeds
    // We will create a memory capability and transfer it over the endpoint.
    uint32_t mem_cap = 0;
    assert(cap_table_grant(table, CAP_TYPE_MEMORY, 0x1000, CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_MEMORY_UNMAP | CAP_RIGHT_DELEGATE, &mem_cap) == 0);

    // Send message with mem_cap attached
    assert(ipc_endpoint_send(table, send_cap, "cap1", 4U, 0, mem_cap, CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_MEMORY_UNMAP) == IPC_OK);

    // Receive message and the transferred cap
    char out_cap_msg[16] = {0};
    uint32_t out_cap_len = 0;
    uint32_t received_mem_cap = 0;
    assert(ipc_endpoint_receive(table, recv_cap, out_cap_msg, sizeof(out_cap_msg), &out_cap_len, 0, &received_mem_cap) == IPC_OK);

    assert(out_cap_len == 4U);
    assert(received_mem_cap != 0 && received_mem_cap != mem_cap); // Must be a newly minted/installed capability ID

    // Verify received capability rights (should be attenuated to exactly what was transferred)
    capability_entry_t e = {0};
    assert(cap_table_lookup(table, received_mem_cap, CAP_TYPE_MEMORY, CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_MEMORY_UNMAP, &e) == 0);
    assert((e.rights & CAP_RIGHT_DELEGATE) == 0); // Sender didn't include DELEGATE

    // 2. Transfer of a non-transferable cap fails
    uint32_t sched_cap = 0;
    assert(cap_table_grant(table, CAP_TYPE_SCHED, 0, CAP_RIGHT_SCHEDULE | CAP_RIGHT_DELEGATE, &sched_cap) == 0);

    // Sending a SCHED capability should fail validation
    assert(ipc_endpoint_send(table, send_cap, "bad", 3U, 0, sched_cap, CAP_RIGHT_SCHEDULE) == IPC_ERR_CAP_TRANSFER_NOT_ALLOWED);

    // 3. Revoking the ancestor invalidates the descendants in the receiver's cap table
    assert(cap_table_revoke(table, mem_cap) == 0);

    // The received_mem_cap should now be invalid because its ancestor (mem_cap) was revoked
    assert(cap_table_lookup(table, received_mem_cap, CAP_TYPE_MEMORY, CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_MEMORY_UNMAP, &e) != 0);

    // 4. Failed receive-side installation leaves message intact (Retryable Receive)

    // Create endpoint for this test
    uint32_t send_cap2, recv_cap2;
    assert(ipc_endpoint_create(table, &send_cap2, &recv_cap2) == IPC_OK);

    // Send a message with capability attached
    uint32_t mem_cap2 = 0;
    assert(cap_table_grant(table, CAP_TYPE_MEMORY, 0x1000, CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_MEMORY_UNMAP | CAP_RIGHT_DELEGATE, &mem_cap2) == 0);
    assert(ipc_endpoint_send(table, send_cap2, "cap2", 4U, 0, mem_cap2, CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_MEMORY_UNMAP) == IPC_OK);

    // Give recv_cap2 to full_table BEFORE filling it up
    capability_table_t* full_table = cap_table_create();
    uint32_t full_table_recv_cap;
    capability_entry_t e2 = {0};
    cap_table_lookup(table, recv_cap2, CAP_TYPE_ENDPOINT, CAP_RIGHT_ENDPOINT_RECEIVE, &e2);
    assert(cap_table_grant(full_table, CAP_TYPE_ENDPOINT, e2.object_ref, CAP_RIGHT_ENDPOINT_RECEIVE | CAP_RIGHT_DELEGATE, &full_table_recv_cap) == 0);

    // We will exhaust the receiver capability table so installation fails.
    uint32_t dummy_cap;
    for (int i = 0; i < 64; i++) {
        // Fill table, we know BHARAT_ARRAY_SIZE(table->entries) is 64
        cap_table_grant(full_table, CAP_TYPE_MEMORY, 0x2000, CAP_RIGHT_MEMORY_MAP, &dummy_cap);
    }

    // Attempt receive into full table (should fail CAP_INSTALL_FAILED)
    char out_cap_msg2[16] = {0};
    uint32_t out_cap_len2 = 0;
    uint32_t received_mem_cap2 = 0;
    int res = ipc_endpoint_receive(full_table, full_table_recv_cap, out_cap_msg2, sizeof(out_cap_msg2), &out_cap_len2, 0, &received_mem_cap2);
    assert(res == IPC_ERR_CAP_INSTALL_FAILED);

    // The message should STILL be in the endpoint. Let's receive it into the normal table which has space.
    assert(ipc_endpoint_receive(table, recv_cap2, out_cap_msg2, sizeof(out_cap_msg2), &out_cap_len2, 0, &received_mem_cap2) == IPC_OK);
    assert(out_cap_len2 == 4U);

    printf("Capability/endpoint IPC tests passed.\n");
    return 0;
}





#include "../kernel/include/slab.h"
#include <stdlib.h>
#include <stdlib.h>

void* kmalloc(size_t size) {
    return malloc(size);
}
void kfree(void* ptr) {
    free(ptr);
}
uint64_t hal_timer_monotonic_ticks(void) {
    return 0;
}
void arch_prepare_initial_context(cpu_context_t* ctx, void (*entry)(void), uint64_t stack_top) {
    (void)ctx;
    (void)entry;
    (void)stack_top;
}
void arch_context_switch(cpu_context_t* prev, cpu_context_t* next) {
    (void)prev;
    (void)next;
}
#include <string.h>

// Some tests were crashing on free() because thread_destroy called kcache_free on pointers that weren't allocated by kcache_alloc (like stack/global variables in tests).
// Since these are stub tests without full memory management setup, let's just make kcache_free a no-op for tests.

uint32_t hal_cpu_get_id(void) { return 0; }
void hal_cpu_halt(void) { }

// Stubs for NUMA page migration dependencies
phys_addr_t pmm_alloc_pages_colored(int order, uint32_t preferred_numa_node, uint32_t flags, mm_color_config_t *color_config) {
    (void)order; (void)preferred_numa_node; (void)flags; (void)color_config;
    return 0;
}
phys_addr_t mm_alloc_pages_order(int order, uint32_t preferred_numa_node, uint32_t flags) {
    (void)order; (void)preferred_numa_node; (void)flags;
    return 0;
}
int hal_vmm_get_mapping(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t* paddr, uint32_t* flags) {
    (void)root_table; (void)vaddr; (void)paddr; (void)flags;
    return -1;
}
int hal_vmm_update_mapping(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    (void)root_table; (void)vaddr; (void)paddr; (void)flags;
    return -1;
}
void tlb_shootdown(address_space_t *as, virt_addr_t vaddr) {
    (void)as;
    (void)vaddr;
}
