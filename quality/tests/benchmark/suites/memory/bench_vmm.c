#include "bench.h"
#include <stdlib.h>
#include <string.h>

static int vmm_setup(void **ctx) {
    // Mock page table allocation setup
    void *pt = malloc(4096 * 512); // L1/L2 tables
    *ctx = pt;
    return pt ? 0 : -1;
}

static int vmm_run_map_unmap(void *ctx, uint64_t iterations) {
    // Mock run representing map/unmap page tables
    void *pt = ctx;
    uint64_t offset = 0;
    for (uint64_t i = 0; i < iterations; i++) {
        // Mock pte write and read
        uint64_t *pte = (uint64_t*)((uint8_t*)pt + offset);
        *pte = 0x80000000ULL | 0x1; // map
        *pte = 0; // unmap
        offset += 8;
        if (offset >= (4096 * 512)) offset = 0;
    }
    return 0;
}

static void vmm_teardown(void *ctx) {
    free(ctx);
}

REGISTER_BENCHMARK(vmm_map_unmap_1pte, "memory", BENCH_LEVEL_B1, 500000, 50000, vmm_setup, vmm_run_map_unmap, vmm_teardown);
