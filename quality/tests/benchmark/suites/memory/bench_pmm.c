#include "bench.h"
#include <stdlib.h>
#include <string.h>

static int pmm_setup(void **ctx) {
    // Mock memory pool allocation setup for host test
    void *pool = malloc(1024 * 1024 * 64); // 64 MB
    *ctx = pool;
    return pool ? 0 : -1;
}

static int pmm_run_alloc_free(void *ctx, uint64_t iterations) {
    // Mock run representing single-page alloc/free cost
    void *pool = ctx;
    uint64_t offset = 0;
    for (uint64_t i = 0; i < iterations; i++) {
        void *page = (uint8_t*)pool + offset;
        offset += 4096;
        if (offset >= (1024 * 1024 * 64)) offset = 0;
        memset(page, 0, 4096);
    }
    return 0;
}

static void pmm_teardown(void *ctx) {
    free(ctx);
}

REGISTER_BENCHMARK(pmm_alloc_free, "memory", BENCH_LEVEL_B1, 100000, 10000, pmm_setup, pmm_run_alloc_free, pmm_teardown);
