#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs/vfs.h"
#include "fs/file.h"
#include "fs/mount.h"
#include "fs/ramfs.h"

// Provide dummy kmalloc/kfree since it's host-side tests
void* kmalloc(size_t size) {
    return malloc(size);
}

void kfree(void* ptr) {
    free(ptr);
}

int main(void) {
    vfs_test_reset_state();

    assert(ramfs_register_driver() == 0);

    vfs_node_t *root_node = ramfs_create_instance();
    assert(root_node != NULL);

    capability_t mount_cap = {
        .target_object_id = VFS_NAMESPACE_OBJECT_ID,
        .rights_mask = 2, // Old CAP_RIGHT_WRITE
    };
    assert(vfs_mount_fs("/", root_node, &mount_cap) == 0);

    // Create a file
    int create_result;
    struct {
        const char *name;
        uint32_t type;
    } create_args = {"hello.txt", 1}; // 1 = file

    int fd;
    capability_t root_cap = {
        .target_object_id = root_node->object_id,
        .rights_mask = 1 | 2, // Old CAP_RIGHT_READ | CAP_RIGHT_WRITE
    };

    assert(vfs_open_file("/", VFS_OPEN_READ | VFS_OPEN_WRITE, &root_cap, &fd) == 0);

    // We can't do ioctl via vfs layer because there is no vfs_ioctl wrapper, but we can call it on the root node's ops
    vfs_file_t file_struct = {
        .node = root_node,
    };
    assert(root_node->ops->ioctl(&file_struct, 1 /* RAMFS_IOCTL_CREATE */, &create_args) == 0);

    // Find the file
    vfs_node_t *hello_node = root_node->ops->finddir(root_node, "hello.txt");
    assert(hello_node != NULL);
    assert(hello_node->flags == 1);

    // Write to the file
    capability_t file_cap = {
        .target_object_id = hello_node->object_id,
        .rights_mask = 1 | 2, // Old CAP_RIGHT_READ | CAP_RIGHT_WRITE
    };

    // We can just call ops directly for testing if we want or via mount resolving
    // Because path resolution for sub-nodes might not be fully wired up in vfs_resolve_mount_path for nested nodes.
    // Wait, vfs_resolve_mount_path only resolves up to the mount point! It doesn't walk the directory tree!
    // If it doesn't walk the directory tree, we have to just open the root, and then maybe we don't have openat.
    // Let's just use ops directly for now to test ramfs backend functions.

    vfs_file_t opened_hello = {
        .node = hello_node,
        .flags = VFS_OPEN_READ | VFS_OPEN_WRITE,
        .offset = 0,
    };

    int wbytes = hello_node->ops->write(&opened_hello, 0, "Hello, Ramfs!", 13);
    assert(wbytes == 13);
    assert(hello_node->size == 13);

    char buf[32] = {0};
    int rbytes = hello_node->ops->read(&opened_hello, 0, buf, 13);
    assert(rbytes == 13);
    assert(memcmp(buf, "Hello, Ramfs!", 13) == 0);

    // Test truncate shrinking
    uint64_t new_size = 5;
    assert(root_node->ops->ioctl(&opened_hello, 3 /* RAMFS_IOCTL_TRUNCATE */, &new_size) == 0);
    assert(hello_node->size == 5);

    // Readdir
    struct dirent *dir = root_node->ops->readdir(&file_struct, 0);
    assert(dir != NULL);
    assert(strcmp(dir->d_name, "hello.txt") == 0);
    dir = root_node->ops->readdir(&file_struct, 1);
    assert(dir == NULL);

    // Unlink the file
    assert(root_node->ops->ioctl(&file_struct, 2 /* RAMFS_IOCTL_UNLINK */, (void*)"hello.txt") == 0);

    // Ensure it's gone
    assert(root_node->ops->finddir(root_node, "hello.txt") == NULL);

    puts("test_vfs_ramfs: PASS");
    return 0;
}
