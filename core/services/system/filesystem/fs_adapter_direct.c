#include "../../../lib/fs/fs_client_ops.h"
#include <stddef.h>

// Direct extern declarations to original filesystem service methods
extern int fsd_open_file(const char* path, int flags, capability_t* caller_cap, int* out_fd);
extern int fsd_openat_file(int dirfd, const char* path, int flags, capability_t* caller_cap, int* out_fd);
extern int fsd_read_file(int fd, void* buffer, size_t size, capability_t* caller_cap);
extern int fsd_write_file(int fd, const void* buffer, size_t size, capability_t* caller_cap);
extern int fsd_close_file(int fd, capability_t* caller_cap);
extern int64_t fsd_lseek_file(int fd, int64_t offset, int whence, capability_t* caller_cap);
extern int fsd_fstat_file(int fd, void* stat_buf, capability_t* caller_cap);
extern int vfs_mount_fs(const char* target_path, vfs_node_t* fs_root, capability_t* caller_cap);
extern int vfs_mkdir(const char* path, int mode, capability_t* caller_cap);
extern struct dirent* vfs_readdir(int fd, uint32_t index, capability_t* caller_cap);
extern int vfs_unlink(const char* path, capability_t* caller_cap);

static int direct_open(const char* path, int flags, capability_t* caller_cap, int* out_fd) {
    return fsd_open_file(path, flags, caller_cap, out_fd);
}

static int direct_openat(int dirfd, const char* path, int flags, capability_t* caller_cap, int* out_fd) {
    return fsd_openat_file(dirfd, path, flags, caller_cap, out_fd);
}

static int direct_read(int fd, void* buffer, size_t size, capability_t* caller_cap) {
    return fsd_read_file(fd, buffer, size, caller_cap);
}

static int direct_write(int fd, const void* buffer, size_t size, capability_t* caller_cap) {
    return fsd_write_file(fd, buffer, size, caller_cap);
}

static int direct_close(int fd, capability_t* caller_cap) {
    return fsd_close_file(fd, caller_cap);
}

static int direct_mount(const char* target_path, vfs_node_t* fs_root, capability_t* caller_cap) {
    return vfs_mount_fs(target_path, fs_root, caller_cap);
}

static int64_t direct_lseek(int fd, int64_t offset, int whence, capability_t* caller_cap) {
    return fsd_lseek_file(fd, offset, whence, caller_cap);
}

static int direct_fstat(int fd, void* stat_buf, capability_t* caller_cap) {
    return fsd_fstat_file(fd, stat_buf, caller_cap);
}

static int direct_mkdir(const char* path, int mode, capability_t* caller_cap) {
    return vfs_mkdir(path, mode, caller_cap);
}

static struct dirent* direct_readdir(int fd, uint32_t index, capability_t* caller_cap) {
    return vfs_readdir(fd, index, caller_cap);
}

static int direct_unlink(const char* path, capability_t* caller_cap) {
    return vfs_unlink(path, caller_cap);
}

static const fs_client_ops_t direct_ops = {
    .open = direct_open,
    .openat = direct_openat,
    .read = direct_read,
    .write = direct_write,
    .close = direct_close,
    .mount = direct_mount,
    .lseek = direct_lseek,
    .fstat = direct_fstat,
    .mkdir = direct_mkdir,
    .readdir = direct_readdir,
    .unlink = direct_unlink,
};

void fs_adapter_direct_init(void) {
    fs_client_set_ops(&direct_ops);
}