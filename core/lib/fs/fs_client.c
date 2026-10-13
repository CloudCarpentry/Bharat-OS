#include "fs_client.h"
#include "fs_client_ops.h"
#include "../../kernel/src/lib/base/status.h" // For ENOSYS or define it

#define ENOSYS 38 // Using standard POSIX ENOSYS if not defined.
// Note: We might need to include errno.h if available, but for now returning -38

static const fs_client_ops_t* g_fs_client_ops = NULL;

void fs_client_set_ops(const fs_client_ops_t* ops) {
    g_fs_client_ops = ops;
}

int fs_open(const char* path, int flags, capability_t* caller_cap, int* out_fd) {
    if (!g_fs_client_ops || !g_fs_client_ops->open) return -ENOSYS;
    return g_fs_client_ops->open(path, flags, caller_cap, out_fd);
}

int fs_openat(int dirfd, const char* path, int flags, capability_t* caller_cap, int* out_fd) {
    if (!g_fs_client_ops || !g_fs_client_ops->openat) return -ENOSYS;
    return g_fs_client_ops->openat(dirfd, path, flags, caller_cap, out_fd);
}

int fs_read(int fd, void* buffer, size_t size, capability_t* caller_cap) {
    if (!g_fs_client_ops || !g_fs_client_ops->read) return -ENOSYS;
    return g_fs_client_ops->read(fd, buffer, size, caller_cap);
}

int fs_write(int fd, const void* buffer, size_t size, capability_t* caller_cap) {
    if (!g_fs_client_ops || !g_fs_client_ops->write) return -ENOSYS;
    return g_fs_client_ops->write(fd, buffer, size, caller_cap);
}

int fs_close(int fd, capability_t* caller_cap) {
    if (!g_fs_client_ops || !g_fs_client_ops->close) return -ENOSYS;
    return g_fs_client_ops->close(fd, caller_cap);
}

int fs_mount(const char* target_path, vfs_node_t* fs_root, capability_t* caller_cap) {
    if (!g_fs_client_ops || !g_fs_client_ops->mount) return -ENOSYS;
    return g_fs_client_ops->mount(target_path, fs_root, caller_cap);
}

int64_t fs_lseek(int fd, int64_t offset, int whence, capability_t* caller_cap) {
    if (!g_fs_client_ops || !g_fs_client_ops->lseek) return -ENOSYS;
    return g_fs_client_ops->lseek(fd, offset, whence, caller_cap);
}

int fs_fstat(int fd, void* stat_buf, capability_t* caller_cap) {
    if (!g_fs_client_ops || !g_fs_client_ops->fstat) return -ENOSYS;
    return g_fs_client_ops->fstat(fd, stat_buf, caller_cap);
}

int fs_mkdir(const char* path, int mode, capability_t* caller_cap) {
    if (!g_fs_client_ops || !g_fs_client_ops->mkdir) return -ENOSYS;
    return g_fs_client_ops->mkdir(path, mode, caller_cap);
}

struct dirent* fs_readdir(int fd, uint32_t index, capability_t* caller_cap) {
    if (!g_fs_client_ops || !g_fs_client_ops->readdir) return NULL;
    return g_fs_client_ops->readdir(fd, index, caller_cap);
}

int fs_unlink(const char* path, capability_t* caller_cap) {
    if (!g_fs_client_ops || !g_fs_client_ops->unlink) return -ENOSYS;
    return g_fs_client_ops->unlink(path, caller_cap);
}
