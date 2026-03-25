#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "fs/blob_backend.h"
#include "fs/vfs.h"
#include "fs/mount.h"
#include "fs/file.h"
#include "capability.h"

int __attribute__((weak)) vfs_mount(const char* path, vfs_node_t* root) { (void)path; (void)root; return -1; }
int __attribute__((weak)) vfs_open(const char* path, int flags) { (void)path; (void)flags; return -1; }
int __attribute__((weak)) vfs_read(int fd, void* buf, size_t count) { (void)fd; (void)buf; (void)count; return -1; }

void* arch_memcpy(void* dest, const void* src, size_t n, uint32_t flags) {
    (void)flags;
    char* d = (char*)dest;
    const char* s = (const char*)src;
    while(n--) *d++ = *s++;
    return dest;
}
void* arch_memset(void* s, int c, size_t n, uint32_t flags) {
    (void)flags;
    char* p = (char*)s;
    while(n--) *p++ = (char)c;
    return s;
}
void* arch_memmove(void* dest, const void* src, size_t n, uint32_t flags) {
    (void)flags;
    char* d = (char*)dest;
    const char* s = (const char*)src;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--) *--d = *--s;
    }
    return dest;
}

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

capability_table_t *g_mock_cap_table;

#include "sched.h"


extern kprocess_t* g_stub_current_process;

int main(void) {
    static kprocess_t proc;
    g_stub_current_process = &proc;
    proc.security_sandbox_ctx = g_mock_cap_table;
    // initialize g_memory_fs
    strcpy((char*)g_memory_fs, "hello-kernel-vfs");

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
    fs_root.object_id = 1;

    capability_table_t test_cap_table;
    memset(&test_cap_table, 0, sizeof(test_cap_table));
    test_cap_table.next_id = 4; // Use something > 3

    g_mock_cap_table = &test_cap_table;


    capability_t dummy_cap = { .rights_mask = CAP_RIGHT_READ | CAP_RIGHT_WRITE, .target_object_id = 1, .capability_id = 0 };

    capability_t mount_cap = {
        .target_object_id = VFS_NAMESPACE_OBJECT_ID,
        .rights_mask = CAP_RIGHT_WRITE,
        .capability_id = 0,
    };

    assert(vfs_mount_fs("/", &fs_root, &mount_cap) == 0);

    assert(blob_backend_register_s3_driver() == 0);
    assert(vfs_get_driver("s3-compatible") != NULL);

    assert(blob_backend_init_immutable_node(&blob_node,
                                            "remote/minio/bucket/key",
                                            blob_data,
                                            sizeof(blob_data) - 1) == 0);
    blob_node.object_id = 2;

    capability_t dummy_blob_cap = { .rights_mask = CAP_RIGHT_READ | CAP_RIGHT_WRITE, .target_object_id = 2, .capability_id = 0 };

    assert(vfs_mount_fs("/blob/remote/minio/bucket/key", &blob_node, &mount_cap) == 0);

    int res = vfs_open_file("/", VFS_OPEN_READ | VFS_OPEN_WRITE, &dummy_cap, &fd);
    if (res != 0) {
        fprintf(stderr, "vfs_open_file failed with %d\n", res);
    }
    assert(res == 0);
    assert(fd >= 0);
    int read_bytes = vfs_read_file(fd, read_buf, 5, &dummy_cap);
    assert(read_bytes == 5);
    assert(memcmp(read_buf, "hello", 5) == 0);

    assert(vfs_open_file("/blob/remote/minio/bucket/key", VFS_OPEN_READ, &dummy_blob_cap, &fd) == 0);
    assert(fd >= 0);
    memset(read_buf, 0, sizeof(read_buf));
    assert(vfs_read_file(fd, read_buf, sizeof(blob_data) - 1, &dummy_blob_cap) == (int)(sizeof(blob_data) - 1));
    assert(memcmp(read_buf, blob_data, sizeof(blob_data) - 1) == 0);

    assert(vfs_open_file("/blob/remote/minio/bucket/key", VFS_OPEN_WRITE, &dummy_blob_cap, &fd) < 0);

    // Re-open blob file for reading but attempt to write to it to ensure VFS block backend checks it
    assert(vfs_open_file("/blob/remote/minio/bucket/key", VFS_OPEN_READ, &dummy_blob_cap, &fd) == 0);
    assert(fd >= 0);

    // Explicit test: "blob write denied even if caller has write right"
    capability_t malicious_blob_write_cap = { .rights_mask = CAP_RIGHT_READ | CAP_RIGHT_WRITE, .target_object_id = 2, .capability_id = 0 };
    assert(vfs_write_file(fd, "test", 4, &malicious_blob_write_cap) == -1);

    puts("test_vfs_storage: PASS");
    return 0;
}
