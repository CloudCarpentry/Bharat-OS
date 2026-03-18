#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "benchmark/benchmark.h"
#include "fs/mount.h"
#include "fs/vfs.h"

// Note: We don't redefine vfs_root here because it's already in vfs.c
// and we are linking against vfs.c

// We need a dummy vfs_file_test_reset_state to satisfy vfs_test_reset_state if we don't link file.c
void vfs_file_test_reset_state(void) {
    // nothing
}

void benchmark_mount_resolution(void) {
    vfs_node_t mock_node = {0};
    capability_t mock_cap = {0};

    // Reset state
    vfs_mount_test_reset_state();

    // Mount some paths to create a bit of a search space
    vfs_mount_fs("/", &mock_node, &mock_cap);
    vfs_mount_fs("/home", &mock_node, &mock_cap);
    vfs_mount_fs("/home/user", &mock_node, &mock_cap);
    vfs_mount_fs("/home/user/docs", &mock_node, &mock_cap);
    vfs_mount_fs("/usr", &mock_node, &mock_cap);
    vfs_mount_fs("/usr/bin", &mock_node, &mock_cap);
    vfs_mount_fs("/usr/local", &mock_node, &mock_cap);
    vfs_mount_fs("/var/log", &mock_node, &mock_cap);
    vfs_mount_fs("/etc", &mock_node, &mock_cap);
    vfs_mount_fs("/tmp", &mock_node, &mock_cap);
    vfs_mount_fs("/dev", &mock_node, &mock_cap);
    vfs_mount_fs("/proc", &mock_node, &mock_cap);
    vfs_mount_fs("/sys", &mock_node, &mock_cap);
    vfs_mount_fs("/mnt", &mock_node, &mock_cap);
    vfs_mount_fs("/media", &mock_node, &mock_cap);
    vfs_mount_fs("/opt", &mock_node, &mock_cap);

    const int ITERATIONS = 1000000; // 1M iterations for a faster but still meaningful test
    const char* path_to_resolve = "/home/user/docs/work/project/file.txt";

    benchmark_ctx_t ctx;
    benchmark_start(&ctx, "VFS Mount Resolution", BENCHMARK_LEVEL_0_REF, ITERATIONS);

    for (int i = 0; i < ITERATIONS; i++) {
        vfs_node_t* node = vfs_resolve_mount_path(path_to_resolve, &mock_cap);
        (void)node;
    }

    benchmark_stop(&ctx);

    benchmark_result_t result;
    benchmark_record(&ctx, &result);
    benchmark_print(&result, ctx.name);
}

int main() {
    printf("VFS Mount Resolution Benchmark\n");
    benchmark_mount_resolution();
    return 0;
}
