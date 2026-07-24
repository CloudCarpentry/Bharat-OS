#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../kernel/include/capability.h"
#include "../kernel/include/ipc_endpoint.h"
#include "../kernel/include/slab.h"

void ipc_async_check_timeouts(uint64_t current_ticks) { (void)current_ticks; }

static address_space_t g_as = { .root_pt = 0x1000U };

address_space_t* mm_create_address_space(void) { return &g_as; }
phys_addr_t mm_alloc_page(uint32_t preferred_numa_node) {
    (void)preferred_numa_node;
    return 0;
}
void mm_free_page(phys_addr_t page) { (void)page; }

void* kmalloc(size_t size) { return malloc(size); }
void kfree(void* ptr) { free(ptr); }
uint64_t hal_timer_monotonic_ticks(void) { return 0; }
void arch_prepare_initial_context(cpu_context_t* ctx, void (*entry)(void), uint64_t stack_top) {
    (void)ctx; (void)entry; (void)stack_top;
}
void arch_context_switch(cpu_context_t* prev, cpu_context_t* next) {
    (void)prev; (void)next;
}
uint32_t hal_cpu_get_id(void) { return 0; }
void hal_cpu_halt(void) {}
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
    (void)as; (void)vaddr;
}
void mm_switch_active_aspace(address_space_t *as) { (void)as; }
void vm_debug_validate_active_tracking(const char *ctx) { (void)ctx; }
void vmm_process_local_urpc_messages(uint32_t core_id) { (void)core_id; }
void* physmap_phys_to_virt(phys_addr_t pa) { (void)pa; return NULL; }

static void test_payload_macro_matches_profile_expectation(void) {
#if defined(Profile_RTOS)
    assert(BHARAT_IPC_ENDPOINT_PAYLOAD_MAX == 64U);
#elif defined(FEATURE_NUMA_AWARE)
    assert(BHARAT_IPC_ENDPOINT_PAYLOAD_MAX == 256U);
#else
    assert(BHARAT_IPC_ENDPOINT_PAYLOAD_MAX == 128U);
#endif
}

static void test_endpoint_pool_limit(void) {
    capability_table_t* tables[16] = {0};
    uint32_t table_count = 0U;
    uint32_t total_created = 0U;

    while (total_created < BHARAT_IPC_MAX_ENDPOINTS) {
        if (table_count == 0U || (total_created % 32U) == 0U) {
            assert(table_count < 16U);
            tables[table_count] = cap_table_create();
            assert(tables[table_count] != NULL);
            table_count++;
        }

        uint32_t send_cap = 0U;
        uint32_t recv_cap = 0U;
        int ret = ipc_endpoint_create(tables[table_count - 1U], &send_cap, &recv_cap);
        assert(ret == IPC_OK);
        total_created++;
    }

    capability_table_t* extra_table = cap_table_create();
    assert(extra_table != NULL);

    uint32_t send_cap = 0U;
    uint32_t recv_cap = 0U;
    assert(ipc_endpoint_create(extra_table, &send_cap, &recv_cap) == IPC_ERR_NO_SPACE);
}

int main(void) {
    sched_init();
    test_payload_macro_matches_profile_expectation();
    test_endpoint_pool_limit();
    printf("test_ipc_endpoint_profile_limits: PASS\n");
    return 0;
}
