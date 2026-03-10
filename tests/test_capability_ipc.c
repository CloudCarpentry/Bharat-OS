#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../kernel/include/capability.h"
#include "../kernel/include/ipc_endpoint.h"

void ipc_async_check_timeouts(uint64_t current_ticks) {
    (void)current_ticks;
}

static address_space_t g_as = { .root_table = 0x1000U };

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

    kprocess_t* proc = process_create("ipc");
    assert(proc != NULL);

    capability_table_t* table = (capability_table_t*)proc->security_sandbox_ctx;
    assert(table != NULL);

    uint32_t send_cap = 0;
    uint32_t recv_cap = 0;
    assert(ipc_endpoint_create(table, &send_cap, &recv_cap) == IPC_OK);

    const char* msg = "hello";
    assert(ipc_endpoint_send(table, send_cap, msg, 5U) == IPC_OK);

    char out[16] = {0};
    uint32_t out_len = 0;
    assert(ipc_endpoint_receive(table, recv_cap, out, sizeof(out), &out_len) == IPC_OK);
    assert(out_len == 5U);
    assert(out[0] == 'h' && out[4] == 'o');

    // Delegation must only narrow rights.
    uint32_t delegated = 0;
    assert(cap_table_delegate(table, table, send_cap, CAP_PERM_SEND, &delegated) == 0);

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
void tlb_shootdown(virt_addr_t vaddr) {
    (void)vaddr;
}
