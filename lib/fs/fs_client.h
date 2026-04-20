#ifndef BHARAT_FS_CLIENT_H
#define BHARAT_FS_CLIENT_H

#include <stddef.h>
#include "fs/vfs.h"

// Phase 2 fs_client layer to enforce boundary between kernel and service

int fs_open(const char* path, int flags, capability_t* caller_cap, int* out_fd);
int fs_read(int fd, void* buffer, size_t size, capability_t* caller_cap);
int fs_write(int fd, const void* buffer, size_t size, capability_t* caller_cap);
int fs_close(int fd, capability_t* caller_cap);
int fs_mount(const char* target_path, vfs_node_t* fs_root, capability_t* caller_cap);

#endif // BHARAT_FS_CLIENT_H
