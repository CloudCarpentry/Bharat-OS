#ifndef BHARAT_FS_SUPERBLOCK_H
#define BHARAT_FS_SUPERBLOCK_H

#include <stdint.h>
#include "fs/vnode.h"
#include "fs/vfs.h"

/*
 * fs_superblock_t: Represents a mounted filesystem volume.
 * Handles the state of an entire filesystem instance.
 */
typedef struct fs_superblock {
    uint32_t magic;              // Filesystem magic number
    uint32_t block_size;         // Size of a logical block
    uint64_t total_blocks;       // Total blocks in the filesystem
    uint64_t free_blocks;        // Free blocks remaining

    // The device vnode backing this filesystem (if block-backed)
    vfs_node_t* dev_node;

    // The root vnode of this filesystem instance
    vfs_node_t* root_node;

    // Driver descriptor
    const vfs_driver_info_t* driver;

    // Filesystem-specific private data
    void* fs_info;
} fs_superblock_t;

#endif // BHARAT_FS_SUPERBLOCK_H
