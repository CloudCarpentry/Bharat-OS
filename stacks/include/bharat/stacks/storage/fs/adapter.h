#pragma once

#include "bharat/stacks/storage/block.h"
#include <stdint.h>
#include <stddef.h>

// Filesystem Adapter Boundary
typedef struct fs_vnode {
    uint64_t inode_num;
    uint32_t mode; // File, Dir, etc.
    uint64_t size;
    void* fs_private; // Adapter specific data
} fs_vnode_t;

typedef struct fs_adapter {
    const char* name;
    int (*mount)(uint32_t device_id, void** out_fs_handle);
    int (*unmount)(void* fs_handle);
    int (*lookup)(void* fs_handle, fs_vnode_t* dir, const char* name, fs_vnode_t** out_vnode);
    int (*read)(void* fs_handle, fs_vnode_t* vnode, uint64_t offset, void* buf, size_t size);
    int (*write)(void* fs_handle, fs_vnode_t* vnode, uint64_t offset, const void* buf, size_t size);
} fs_adapter_t;

int fs_adapter_register(fs_adapter_t* adapter);
const fs_adapter_t* fs_adapter_get(const char* name);
