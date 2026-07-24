#include "fs/file.h"
#include "kernel/status.h"

// PHASE1_COMPAT_SHIM: File descriptor and capabilities access have moved to services/system/filesystem/.
// These are minimal compatibility shims to keep the kernel building.
// They will eventually forward to the FS service via IPC/uRPC.

int vfs_open_file(const char* path, int flags, capability_t* caller_cap, int* out_fd) {
    (void)path;
    (void)flags;
    (void)caller_cap;
    if (out_fd) *out_fd = -1;
    // Should uRPC to FS Service which allocates and mints FDs mapped to capabilities
    return K_ERR_REQUIRES_FS_SERVICE;
}

int vfs_open(const char* path, int flags) {
    (void)path;
    (void)flags;
    return K_ERR_REQUIRES_FS_SERVICE;
}

int vfs_read_file(int fd, void* buffer, size_t size, capability_t* caller_cap) {
    (void)fd;
    (void)buffer;
    (void)size;
    (void)caller_cap;
    // Should uRPC/SharedMem transfer to Storage Stack
    return K_ERR_REQUIRES_FS_SERVICE;
}

int vfs_read(int fd, void* buffer, size_t size) {
    (void)fd;
    (void)buffer;
    (void)size;
    return K_ERR_REQUIRES_FS_SERVICE;
}

int vfs_write_file(int fd, const void* buffer, size_t size, capability_t* caller_cap) {
    (void)fd;
    (void)buffer;
    (void)size;
    (void)caller_cap;
    // Should uRPC/SharedMem transfer to Storage Stack
    return K_ERR_REQUIRES_FS_SERVICE;
}

int vfs_write(int fd, const void* buffer, size_t size) {
    (void)fd;
    (void)buffer;
    (void)size;
    return K_ERR_REQUIRES_FS_SERVICE;
}

int vfs_close_file(int fd, capability_t* caller_cap) {
    (void)fd;
    (void)caller_cap;
    return K_ERR_REQUIRES_FS_SERVICE;
}

int vfs_close(int fd) {
    (void)fd;
    return K_ERR_REQUIRES_FS_SERVICE;
}

#ifdef TESTING
void vfs_file_test_reset_state(void) {
    // Stub
}
#endif
