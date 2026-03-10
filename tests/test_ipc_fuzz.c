#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void test_ipc_fuzzing(void) {
    printf("Running deterministic IPC fuzzing test...\n");

    kprocess_t* p = process_create("fuzzer");
    assert(p != NULL);

    capability_table_t* t = (capability_table_t*)p->security_sandbox_ctx;
    assert(t != NULL);

    uint32_t send_cap = 0, recv_cap = 0;
    assert(ipc_endpoint_create(t, &send_cap, &recv_cap) == 0);

    unsigned int seed = 12345;

    for (int i = 0; i < 1000; i++) {
        uint32_t payload_len = rand_r(&seed) % 1024;
        uint8_t* payload = malloc(payload_len);
        if (payload) {
            for (uint32_t j = 0; j < payload_len; j++) {
                payload[j] = rand_r(&seed) & 0xFF;
            }

            // Fuzz send
            int ret = ipc_endpoint_send(t, send_cap, payload, payload_len);

            // Fuzz receive with varying buffer sizes
            uint32_t out_len = rand_r(&seed) % 1024;
            uint8_t* out = malloc(out_len);
            if (out) {
                uint32_t actual_received = 0;
                int recv_ret = ipc_endpoint_receive(t, recv_cap, out, out_len, &actual_received);
                (void)ret; (void)recv_ret; // Silence warnings, we just want to ensure it doesn't crash
                free(out);
            }
            free(payload);
        }

        // Fuzz invalid capabilities
        uint32_t bad_cap = rand_r(&seed) % 10000;
        ipc_endpoint_send(t, bad_cap, "test", 4);
    }

    printf("IPC fuzzing test completed without crashing.\n");
}

int main(void) {
    sched_init();
    test_ipc_fuzzing();
    return 0;
}
