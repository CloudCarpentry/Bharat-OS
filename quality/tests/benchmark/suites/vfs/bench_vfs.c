#include "bench.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

static int vfs_setup(void **ctx) {
    // Mock VFS ramfs setup
    void *ramfs_buffer = malloc(1024 * 1024 * 16); // 16 MB
    *ctx = ramfs_buffer;
    return ramfs_buffer ? 0 : -1;
}

static int vfs_run_read_write(void *ctx, uint64_t iterations) {
    // Mock run representing metadata + data IO
    void *ramfs = ctx;
    uint64_t offset = 0;
    for (uint64_t i = 0; i < iterations; i++) {
        void *file_block = (uint8_t*)ramfs + offset;
        offset += 512;
        if (offset >= (1024 * 1024 * 16)) offset = 0;
        memset(file_block, 0xAB, 512);
        volatile uint8_t val = ((uint8_t*)file_block)[0]; // read back
        (void)val;
    }
    return 0;
}

static void vfs_teardown(void *ctx) {
    free(ctx);
}

REGISTER_BENCHMARK(vfs_read_write_512b, "vfs", BENCH_LEVEL_B1, 500000, 50000, vfs_setup, vfs_run_read_write, vfs_teardown);
