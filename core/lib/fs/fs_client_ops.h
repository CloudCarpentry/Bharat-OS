#ifndef BHARAT_FS_CLIENT_OPS_H
#define BHARAT_FS_CLIENT_OPS_H

#include "fs_client.h"

// Forward declaration of capability_t if not included via fs_client.h
// though fs_client.h includes fs/vfs.h which brings it in.

/**
 * The filesystem client operations interface.
 * Abstracts the transport layer for filesystem requests (e.g. direct calls, IPC).
 */
typedef struct {
    int (*open)(const char* path, int flags, capability_t* caller_cap, int* out_fd);
    int (*openat)(int dirfd, const char* path, int flags, capability_t* caller_cap, int* out_fd);
    int (*read)(int fd, void* buffer, size_t size, capability_t* caller_cap);
    int (*write)(int fd, const void* buffer, size_t size, capability_t* caller_cap);
    int (*close)(int fd, capability_t* caller_cap);
    int (*mount)(const char* target_path, vfs_node_t* fs_root, capability_t* caller_cap);
    int64_t (*lseek)(int fd, int64_t offset, int whence, capability_t* caller_cap);
    int (*fstat)(int fd, void* stat_buf, capability_t* caller_cap);
    int (*mkdir)(const char* path, int mode, capability_t* caller_cap);
    struct dirent* (*readdir)(int fd, uint32_t index, capability_t* caller_cap);
    int (*unlink)(const char* path, capability_t* caller_cap);
} fs_client_ops_t;

/**
 * Register the active filesystem client operations adapter.
 *
 * @param ops Pointer to the operations struct. Must outlive the application (usually static).
 */
void fs_client_set_ops(const fs_client_ops_t* ops);

#endif // BHARAT_FS_CLIENT_OPS_H
