#include "fs_client.h"

// Forward logic to userspace filesystem daemon via IPC.
// For tests running in process, this will call the service functions directly
// or tests will mock this layer out until true IPC is set up.

// As of Phase 2, we directly forward to services/system/filesystem implementations
// to allow tests to run without the kernel shims.
// We declare the signatures of the filesystem service functions here.

extern int fsd_open_file(const char* path, int flags, capability_t* caller_cap, int* out_fd);
extern int fsd_openat_file(int dirfd, const char* path, int flags, capability_t* caller_cap, int* out_fd);
extern int fsd_read_file(int fd, void* buffer, size_t size, capability_t* caller_cap);
extern int fsd_write_file(int fd, const void* buffer, size_t size, capability_t* caller_cap);
extern int fsd_close_file(int fd, capability_t* caller_cap);
extern int vfs_mount_fs(const char* target_path, vfs_node_t* fs_root, capability_t* caller_cap);


int fs_open(const char* path, int flags, capability_t* caller_cap, int* out_fd) {
    return fsd_open_file(path, flags, caller_cap, out_fd);
}

int fs_openat(int dirfd, const char* path, int flags, capability_t* caller_cap, int* out_fd) {
    return fsd_openat_file(dirfd, path, flags, caller_cap, out_fd);
}

int fs_read(int fd, void* buffer, size_t size, capability_t* caller_cap) {
    return fsd_read_file(fd, buffer, size, caller_cap);
}

int fs_write(int fd, const void* buffer, size_t size, capability_t* caller_cap) {
    return fsd_write_file(fd, buffer, size, caller_cap);
}

int fs_close(int fd, capability_t* caller_cap) {
    return fsd_close_file(fd, caller_cap);
}

int fs_mount(const char* target_path, vfs_node_t* fs_root, capability_t* caller_cap) {
    return vfs_mount_fs(target_path, fs_root, caller_cap);
}
