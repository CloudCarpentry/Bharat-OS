#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "benchmark/benchmark.h"
#include "ipc_endpoint.h"
#include "capability.h"
#include "sched.h"

// Mock device manager things needed for compilation
uint32_t g_num_drivers = 0;
uint32_t g_num_windows = 0;

void test_ipc_benchmark(void) {
    capability_table_t* table = cap_table_create();
    assert(table != NULL);

    uint32_t send_cap, recv_cap;
    int res = ipc_endpoint_create(table, &send_cap, &recv_cap);
    assert(res == 0);

    const int ITERATIONS = 100000;
    uint8_t payload[64] = "Hello, Benchmark!";
    uint32_t payload_len = strlen((char*)payload) + 1;

    benchmark_ctx_t ctx;
    benchmark_start(&ctx, "IPC Send/Receive", BENCHMARK_LEVEL_0_REF, ITERATIONS);

    for (int i = 0; i < ITERATIONS; i++) {
        res = ipc_endpoint_send(table, send_cap, payload, payload_len);
        assert(res == IPC_OK);

        uint8_t recv_payload[64];
        uint32_t recv_len = 0;
        res = ipc_endpoint_receive(table, recv_cap, recv_payload, sizeof(recv_payload), &recv_len);
        assert(res == IPC_OK);
        assert(recv_len == payload_len);
    }

    benchmark_stop(&ctx);

    benchmark_result_t result;
    benchmark_record(&ctx, &result);
    benchmark_print(&result, ctx.name);

    // Test done
}

int main(void) {
    printf("Running IPC Benchmarks...\n");
    test_ipc_benchmark();
    return 0;
}

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
