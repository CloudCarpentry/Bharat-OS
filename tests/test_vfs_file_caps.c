#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "fs/vfs.h"
#include "fs/file.h"
#include "fs/mount.h"

uint8_t g_memory_fs[1024] = {0};

static int mem_read(vfs_file_t *file, uint64_t offset, void *buffer, size_t size) {
    const char *src = (const char *)g_memory_fs;
    size_t len = strlen(src);
    if (offset >= len) {
        return 0;
    }
    if (size > len - (size_t)offset) {
        size = len - (size_t)offset;
    }
    memcpy(buffer, src + offset, size);
    return (int)size;
}

static int mem_write(vfs_file_t *file, uint64_t offset, const void *buffer, size_t size) {
    char *dst = (char *)g_memory_fs;
    memcpy(dst + offset, buffer, size);
    return (int)size;
}

int main(void) {
    vfs_node_t fs_root = {0};
    vfs_operations_t fs_ops = {
        .read = mem_read,
        .write = mem_write,
        .open = NULL,
        .close = NULL,
        .readdir = NULL,
        .finddir = NULL,
        .ioctl = NULL,
    };
    char fs_data[64] = "hello-kernel-vfs";
    char read_buf[64] = {0};
    int fd;

    vfs_test_reset_state();

    fs_root.backend_type = VFS_BACKEND_FILESYSTEM;
    fs_root.ops = &fs_ops;
    fs_root.fs_data = fs_data;
    fs_root.object_id = 42;

    capability_t mount_cap = {0};
    assert(vfs_mount_fs("/", &fs_root, &mount_cap) == 0);

    capability_t good_cap = {
        .capability_id = 1,
        .target_object_id = 42,
        .rights_mask = CAP_RIGHT_READ | CAP_RIGHT_WRITE,
        .owner_core_id = 0,
    };

    capability_t bad_object_cap = {
        .capability_id = 2,
        .target_object_id = 99,
        .rights_mask = CAP_RIGHT_READ | CAP_RIGHT_WRITE,
        .owner_core_id = 0,
    };

    capability_t bad_rights_cap = {
        .capability_id = 3,
        .target_object_id = 42,
        .rights_mask = 0,
        .owner_core_id = 0,
    };

    assert(vfs_open_file("/", VFS_OPEN_READ | VFS_OPEN_WRITE, &good_cap, &fd) == 0);
    assert(fd >= 0);

    // Write should be denied when capability lacks right
    assert(vfs_write_file(fd, "test", 4, &bad_rights_cap) == -1);

    // Write should be denied when capability targets different object
    assert(vfs_write_file(fd, "test", 4, &bad_object_cap) == -1);

    // Write should succeed with good cap
    assert(vfs_write_file(fd, "test", 4, &good_cap) == 4);

    // Read should be denied when capability lacks right
    assert(vfs_read_file(fd, read_buf, 4, &bad_rights_cap) == -1);

    // Read should be denied when capability targets different object
    assert(vfs_read_file(fd, read_buf, 4, &bad_object_cap) == -1);

    // Read should succeed with good cap
    // Note: since our mem_write just copies to g_memory_fs without updating any size metadata,
    // we should make sure mem_read reads what we wrote, though we might read 0 if the size check fails.
    // wait, mem_read checks `offset >= len`, where `len = strlen(src);`.
    // so if the buffer has null bytes initially and we write "test", strlen is 4!
    // wait, g_memory_fs was initialized to {0}, so strlen(g_memory_fs) is 0 before writing,
    // and after writing "test", strlen(g_memory_fs) is 4.
    // but the fd offset is already 4 after writing!
    // So if offset=4, and len=4, `offset >= len` is true, so it returns 0!
    // Let's reset the offset before reading. Oh wait, we don't have vfs_lseek.
    // Let's open another fd for reading.

    int read_fd;
    assert(vfs_open_file("/", VFS_OPEN_READ, &good_cap, &read_fd) == 0);
    assert(vfs_read_file(read_fd, read_buf, 4, &good_cap) == 4);
    assert(memcmp(read_buf, "test", 4) == 0);
    assert(vfs_close_file(read_fd, &good_cap) == 0);

    // Close should be denied for bad object
    assert(vfs_close_file(fd, &bad_object_cap) == -1);

    // Close should succeed with good cap
    assert(vfs_close_file(fd, &good_cap) == 0);

    puts("test_vfs_file_caps: PASS");
    return 0;
}
