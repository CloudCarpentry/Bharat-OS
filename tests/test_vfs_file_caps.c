#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "fs/file.h"
#include "fs/vfs.h"
#include "fs/mount.h"

int __attribute__((weak)) vfs_mount(const char* path, vfs_node_t* root) { (void)path; (void)root; return -1; }

static int mock_read(vfs_file_t* file, uint64_t offset, void* buffer, size_t size) {
    (void)file; (void)offset; (void)buffer; (void)size;
    return (int)size;
}

static int mock_write(vfs_file_t* file, uint64_t offset, const void* buffer, size_t size) {
    (void)file; (void)offset; (void)buffer; (void)size;
    return (int)size;
}

static int mock_open(vfs_node_t* node, vfs_file_t* file, int flags) {
    (void)node; (void)file; (void)flags;
    return 0;
}

static int mock_close(vfs_file_t* file) {
    (void)file;
    return 0;
}

static vfs_operations_t mock_ops = {
    .read = mock_read,
    .write = mock_write,
    .open = mock_open,
    .close = mock_close,
};

static vfs_node_t mock_node = {
    .object_id = 42,
    .backend_type = VFS_BACKEND_FILESYSTEM,
    .ops = &mock_ops,
};

void test_vfs_file_caps_write() {
    int fd = -1;
    capability_t open_cap = { .rights_mask = CAP_RIGHT_READ | CAP_RIGHT_WRITE, .target_object_id = 42 };

    // Mount the mock node so vfs_resolve_mount_path can find it
    assert(vfs_mount_fs("/testfile", &mock_node, &open_cap) == 0);

    assert(vfs_open_file("/testfile", VFS_OPEN_WRITE, &open_cap, &fd) == 0);
    assert(fd >= 0);

    char buf[10] = "data";

    // 1. Deny when caller_cap == NULL
    assert(vfs_write_file(fd, buf, 4, NULL) == -1);

    // 2. Deny when capability lacks CAP_RIGHT_WRITE
    capability_t bad_rights_cap = { .rights_mask = CAP_RIGHT_READ, .target_object_id = 42 };
    assert(vfs_write_file(fd, buf, 4, &bad_rights_cap) == -1);

    // 3. Deny when capability has write right but targets the wrong object
    capability_t bad_obj_cap = { .rights_mask = CAP_RIGHT_WRITE, .target_object_id = 99 };
    assert(vfs_write_file(fd, buf, 4, &bad_obj_cap) == -1);

    // 4. Allow when capability has write right and matches the file object
    capability_t good_cap = { .rights_mask = CAP_RIGHT_WRITE, .target_object_id = 42 };
    assert(vfs_write_file(fd, buf, 4, &good_cap) == 4);

    // Cleanup
    assert(vfs_close_file(fd, &good_cap) == 0);
}

void test_vfs_file_caps_read() {
    int fd = -1;
    capability_t open_cap = { .rights_mask = CAP_RIGHT_READ | CAP_RIGHT_WRITE, .target_object_id = 42 };

    assert(vfs_open_file("/testfile", VFS_OPEN_READ, &open_cap, &fd) == 0);
    assert(fd >= 0);

    char buf[10];

    // 1. Deny when caller_cap == NULL
    assert(vfs_read_file(fd, buf, 4, NULL) == -1);

    // 2. Deny when capability lacks CAP_RIGHT_READ
    capability_t bad_rights_cap = { .rights_mask = CAP_RIGHT_WRITE, .target_object_id = 42 };
    assert(vfs_read_file(fd, buf, 4, &bad_rights_cap) == -1);

    // 3. Deny when capability has read right but targets the wrong object
    capability_t bad_obj_cap = { .rights_mask = CAP_RIGHT_READ, .target_object_id = 99 };
    assert(vfs_read_file(fd, buf, 4, &bad_obj_cap) == -1);

    // 4. Allow when capability has read right and matches the file object
    capability_t good_cap = { .rights_mask = CAP_RIGHT_READ, .target_object_id = 42 };
    assert(vfs_read_file(fd, buf, 4, &good_cap) == 4);

    // Cleanup
    assert(vfs_close_file(fd, &good_cap) == 0);
}

void test_vfs_file_caps_close() {
    int fd = -1;
    capability_t open_cap = { .rights_mask = CAP_RIGHT_READ | CAP_RIGHT_WRITE, .target_object_id = 42 };

    // We open twice so we can test close failure on the first without tearing down,
    // but a single fd is fine since it doesn't actually close if validation fails.
    assert(vfs_open_file("/testfile", VFS_OPEN_READ, &open_cap, &fd) == 0);
    assert(fd >= 0);

    // 1. Deny when caller_cap == NULL
    assert(vfs_close_file(fd, NULL) == -1);

    // 2. Deny when capability targets the wrong object
    capability_t bad_obj_cap = { .rights_mask = CAP_RIGHT_READ | CAP_RIGHT_WRITE, .target_object_id = 99 };
    assert(vfs_close_file(fd, &bad_obj_cap) == -1);

    // 3. Allow when capability matches the file object
    capability_t good_cap = { .rights_mask = CAP_RIGHT_READ, .target_object_id = 42 };
    assert(vfs_close_file(fd, &good_cap) == 0);
}

int main(void) {
    vfs_file_test_reset_state();

    test_vfs_file_caps_write();
    test_vfs_file_caps_read();
    test_vfs_file_caps_close();

    puts("test_vfs_file_caps: PASS");
    return 0;
}
