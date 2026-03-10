#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../kernel/include/capability.h"
#include "../kernel/include/ipc_endpoint.h"
#include "../kernel/include/sched.h"

// Stubs for dependencies
address_space_t g_as = { .root_table = 0x1000U };
address_space_t* mm_create_address_space(void) { return &g_as; }
phys_addr_t mm_alloc_page(uint32_t preferred_numa_node) { (void)preferred_numa_node; return 0; }
void mm_free_page(phys_addr_t page) { (void)page; }
void tlb_shootdown(virt_addr_t vaddr) { (void)vaddr; }
int hal_vmm_get_mapping(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t* paddr, uint32_t* flags) {
    (void)root_table; (void)vaddr; (void)paddr; (void)flags; return -1;
}
int hal_vmm_update_mapping(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    (void)root_table; (void)vaddr; (void)paddr; (void)flags; return -1;
}
phys_addr_t pmm_alloc_pages_colored(int order, uint32_t preferred_numa_node, uint32_t flags, mm_color_config_t *color_config) {
    (void)order; (void)preferred_numa_node; (void)flags; (void)color_config; return 0;
}
phys_addr_t mm_alloc_pages_order(int order, uint32_t preferred_numa_node, uint32_t flags) {
    (void)order; (void)preferred_numa_node; (void)flags; return 0;
}
uint32_t hal_cpu_get_id(void) { return 0; }
void hal_cpu_halt(void) { }
void ipc_async_check_timeouts(uint64_t current_ticks) { (void)current_ticks; }
uint64_t hal_timer_monotonic_ticks(void) { return 0; }

int vmm_map_device_mmio(virt_addr_t vaddr, phys_addr_t paddr, capability_t* cap, int is_npu) {
    (void)vaddr; (void)paddr; (void)cap; (void)is_npu;
    return 0;
}

static void test_ipc_bypass(void) {
    kprocess_t* p = process_create("p1");
    assert(p != NULL);
    capability_table_t* t = (capability_table_t*)p->security_sandbox_ctx;

    kprocess_t* p2 = process_create("p2");
    assert(p2 != NULL);
    capability_table_t* t2 = (capability_table_t*)p2->security_sandbox_ctx;

    uint32_t send_cap = 0, recv_cap = 0;
    // p1 creates IPC channel
    assert(ipc_endpoint_create(t, &send_cap, &recv_cap) == 0);

    // p1 tries to use p2's table (cross task privilege escalation)
    // Send a message using another capability table should fail (simulating trying to read an unauthorized cap)
    // Actually, ipc_endpoint_send relies on the passed table.
    // Let's assert that the capability is not found in t2.
    int ret = ipc_endpoint_send(t2, send_cap, "hello", 5);
    assert(ret == IPC_ERR_PERM || ret == IPC_ERR_INVALID);

    // Forged Handle Test
    uint32_t forged_cap = send_cap + 100; // Unlikely to be valid
    ret = ipc_endpoint_send(t, forged_cap, "hello", 5);
    assert(ret == IPC_ERR_PERM || ret == IPC_ERR_INVALID);
}

static void test_device_access(void) {
    kprocess_t* p = process_create("device_process");
    assert(p != NULL);
    capability_table_t* t = (capability_table_t*)p->security_sandbox_ctx;

    uint32_t cap = 0;
    // Grant endpoint rights, but try to use it as a device (capability confusion)
    assert(cap_table_grant(t, CAP_OBJ_ENDPOINT, 1U, CAP_PERM_RECEIVE, &cap) == 0);

    // Check if cap is accessible as device
    capability_entry_t resolved;
    int err = cap_table_lookup(t, cap, CAP_OBJ_ENDPOINT, CAP_PERM_RECEIVE, &resolved);
    assert(err == 0);

    // Ensure the resolved capability is NOT a device
    assert((resolved.rights & CAP_RIGHT_DEVICE_NPU) == 0);
    assert((resolved.rights & CAP_RIGHT_DEVICE_GPU) == 0);
}

int main(void) {
    sched_init();

    printf("Running security isolation tests...\n");
    test_ipc_bypass();
    test_device_access();
    printf("Security isolation tests passed successfully.\n");

    return 0;
}
