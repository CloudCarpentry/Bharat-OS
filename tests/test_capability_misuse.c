#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../kernel/include/capability.h"
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





void test_accelerator_caps(void);

int main(void) {
    sched_init();
    kprocess_t* p = process_create("cap-misuse");
    assert(p != NULL);

    cap_table_init_for_process(p);

    capability_table_t* t = (capability_table_t*)p->security_sandbox_ctx;
    assert(t != NULL);

    uint32_t cap = 0;

    // Invalid rights for endpoint object should fail.
    assert(cap_table_grant(t, CAP_TYPE_ENDPOINT, 1U, CAP_RIGHT_MAP, &cap) != 0);

    // Valid grant succeeds.
    assert(cap_table_grant(t, CAP_TYPE_ENDPOINT, 1U, CAP_RIGHT_SEND | CAP_RIGHT_DELEGATE, &cap) == 0);

    // Delegating unsupported rights should fail.
    uint32_t delegated = 0;
    assert(cap_table_delegate(t, t, cap, CAP_RIGHT_MAP, &delegated) != 0);

    printf("Capability misuse tests passed.\n");

    test_accelerator_caps();
    return 0;
}





#include "../kernel/include/slab.h"
#include <stdlib.h>
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

void test_accelerator_caps(void) {
    kprocess_t* p = process_create("accel-test");
    assert(p != NULL);
    cap_table_init_for_process(p);
    capability_table_t* t = (capability_table_t*)p->security_sandbox_ctx;
    assert(t != NULL);

    uint32_t cap_device = 0;
    uint32_t cap_queue = 0;
    uint32_t cap_buffer = 0;
    uint32_t cap_telemetry = 0;
    uint32_t cap_admin = 0;
    uint32_t cap_dma_domain = 0;
    uint32_t cap_dma_grant = 0;

    // Test correct grants
    assert(cap_table_grant(t, CAP_TYPE_ACCEL_DEVICE, 1U, CAP_RIGHT_ALL | CAP_RIGHT_DELEGATE, &cap_device) == 0);
    assert(cap_table_grant(t, CAP_TYPE_ACCEL_QUEUE, 2U, CAP_RIGHT_ENQUEUE | CAP_RIGHT_CANCEL | CAP_RIGHT_DELEGATE, &cap_queue) == 0);
    assert(cap_table_grant(t, CAP_TYPE_ACCEL_BUFFER, 3U, CAP_RIGHT_MAP | CAP_RIGHT_BIND | CAP_RIGHT_DELEGATE, &cap_buffer) == 0);
    assert(cap_table_grant(t, CAP_TYPE_ACCEL_TELEMETRY, 4U, CAP_RIGHT_READ_STATS | CAP_RIGHT_READ_FAULTS | CAP_RIGHT_DELEGATE, &cap_telemetry) == 0);
    assert(cap_table_grant(t, CAP_TYPE_ACCEL_ADMIN, 5U, CAP_RIGHT_RESET | CAP_RIGHT_PARTITION | CAP_RIGHT_DELEGATE, &cap_admin) == 0);
    assert(cap_table_grant(t, CAP_TYPE_DMA_DOMAIN, 6U, CAP_RIGHT_ALL | CAP_RIGHT_DELEGATE, &cap_dma_domain) == 0);
    assert(cap_table_grant(t, CAP_TYPE_DMA_GRANT, 7U, CAP_RIGHT_MAP | CAP_RIGHT_REVOKE | CAP_RIGHT_DELEGATE, &cap_dma_grant) == 0);

    // Test incorrect rights
    uint32_t bad_cap = 0;
    assert(cap_table_grant(t, CAP_TYPE_ACCEL_QUEUE, 8U, CAP_RIGHT_MAP, &bad_cap) != 0); // Queue shouldn't have MAP right
    assert(cap_table_grant(t, CAP_TYPE_ACCEL_BUFFER, 9U, CAP_RIGHT_ENQUEUE, &bad_cap) != 0); // Buffer shouldn't have ENQUEUE right
    assert(cap_table_grant(t, CAP_TYPE_DMA_GRANT, 10U, CAP_RIGHT_READ_STATS, &bad_cap) != 0); // DMA Grant shouldn't have READ_STATS right

    // Test Delegation
    uint32_t delegated_queue = 0;
    assert(cap_table_delegate(t, t, cap_queue, CAP_RIGHT_ENQUEUE, &delegated_queue) == 0);
    assert(cap_table_delegate(t, t, cap_queue, CAP_RIGHT_MAP, &delegated_queue) != 0); // Delegate bad right

    printf("Accelerator & DMA Capability tests passed.\n");
}
