#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "fs/blob_backend.h"
#include "fs/vfs.h"

static int mem_read(vfs_node_t *node, uint64_t offset, void *buffer, size_t size) {
    const char *src = (const char *)node->fs_data;
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

static int mem_write(vfs_node_t *node, uint64_t offset, const void *buffer, size_t size) {
    char *dst = (char *)node->fs_data;
    memcpy(dst + offset, buffer, size);
    return (int)size;
}

int main(void) {
    vfs_node_t fs_root = {0};
    vfs_node_t blob_node = {0};
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
    static const char blob_data[] = "immutable-object";
    int fd;

    vfs_test_reset_state();

    fs_root.backend_type = VFS_BACKEND_FILESYSTEM;
    fs_root.ops = &fs_ops;
    fs_root.fs_data = fs_data;

    assert(vfs_mount("/", &fs_root) == 0);

    assert(blob_backend_register_s3_driver() == 0);
    assert(vfs_get_driver("s3-compatible") != NULL);

    assert(blob_backend_init_immutable_node(&blob_node,
                                            "remote/minio/bucket/key",
                                            blob_data,
                                            sizeof(blob_data) - 1) == 0);

    assert(vfs_mount("/blob/remote/minio/bucket/key", &blob_node) == 0);

    fd = vfs_open("/", VFS_OPEN_READ | VFS_OPEN_WRITE);
    assert(fd >= 0);
    assert(vfs_read(fd, read_buf, 5) == 5);
    assert(memcmp(read_buf, "hello", 5) == 0);

    fd = vfs_open("/blob/remote/minio/bucket/key", VFS_OPEN_READ);
    assert(fd >= 0);
    memset(read_buf, 0, sizeof(read_buf));
    assert(vfs_read(fd, read_buf, sizeof(blob_data) - 1) == (int)(sizeof(blob_data) - 1));
    assert(memcmp(read_buf, blob_data, sizeof(blob_data) - 1) == 0);

    fd = vfs_open("/blob/remote/minio/bucket/key", VFS_OPEN_WRITE);
    assert(fd < 0);

    puts("test_vfs_storage: PASS");
    return 0;
}
