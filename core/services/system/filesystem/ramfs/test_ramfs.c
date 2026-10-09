#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "fs/vfs.h"
#include "fs/mount.h"

// Mock for vfs_register_driver
static vfs_driver_info_t g_driver_info;
int vfs_register_driver(const vfs_driver_info_t* info) {
    g_driver_info = *info;
    return 0;
}

// Include the C file to access static methods for unit testing
#include "ramfs.c"

int main() {
    ramfs_init();

    // 1. Test long filename truncation avoidance
    const char *long_name = "this_is_a_very_long_filename_that_exceeds_the_typical_limits_of_our_small_static_buffer_which_is_256_bytes_long_"
                            "this_is_a_very_long_filename_that_exceeds_the_typical_limits_of_our_small_static_buffer_which_is_256_bytes_long_"
                            "this_is_a_very_long_filename_that_exceeds_the_typical_limits_of_our_small_static_buffer_which_is_256_bytes_long_"; // > 256 bytes

    ramfs_internal_node_t *long_node = create_ramfs_node(long_name, 0);
    assert(long_node == NULL); // Should fail safely and return NULL, not truncate

    ramfs_internal_node_t *short_node = create_ramfs_node("short_name.txt", 0);
    assert(short_node != NULL);

    // 2. Test getattr unsupported fallback
    int attr_res = ramfs_getattr(&short_node->vnode, NULL);
    assert(attr_res == -38); // Fails on not supported

    int dummy_stat;
    attr_res = ramfs_getattr(&short_node->vnode, &dummy_stat);
    assert(attr_res == -38); // Fails on not supported

    // 3. Test integer overflow in write
    vfs_file_t file = {0};
    file.node = &short_node->vnode;
    int write_res = ramfs_write(&file, (uint64_t)-5, "data", 10);
    assert(write_res == -1); // Should fail due to overflow

    // 4. Test sparse write gap initialization
    write_res = ramfs_write(&file, 10, "data", 4);
    assert(write_res == 4);
    assert(short_node->vnode.size == 14);
    assert(short_node->data[0] == 0); // first bytes should be zeroed
    assert(short_node->data[9] == 0);
    assert(memcmp(short_node->data + 10, "data", 4) == 0);

    // 5. Empty write
    write_res = ramfs_write(&file, 14, "", 0);
    assert(write_res == 0);

    // 6. Test allocation failure handling
    // Not easy to test without mocking realloc, but logic is there

    printf("RAMFS standalone test completed successfully.\n");
    return 0;
}
