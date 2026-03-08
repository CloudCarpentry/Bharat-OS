#ifndef BHARAT_VFS_H
#define BHARAT_VFS_H

#include <stdint.h>
#include <stddef.h>

/*
 * Bharat-OS Virtual File System (VFS)
 * Provides a unified namespace for disparate file systems (ext4, BFS, FAT32)
 * and pseudo-devices (/dev, /proc).
 */

typedef struct vfs_node vfs_node_t;

#define VFS_OPEN_READ  0x1
#define VFS_OPEN_WRITE 0x2
#define VFS_OPEN_RDWR  (VFS_OPEN_READ | VFS_OPEN_WRITE)

// Backing storage class for a node/mount
typedef enum {
    VFS_BACKEND_FILESYSTEM = 0,
    VFS_BACKEND_BLOCK,
    VFS_BACKEND_BLOB,
    VFS_BACKEND_PSEUDO
} vfs_backend_type_t;

// Mount feature capabilities for FS and storage providers
typedef struct {
    uint32_t supports_journaling;
    uint32_t supports_snapshots;
    uint32_t supports_xattrs;
    uint32_t supports_acls;
    uint32_t supports_posix_hardlinks;
    uint32_t supports_sparse_files;
    uint32_t supports_mmap;
} vfs_feature_caps_t;

// Driver descriptor used by VFS registration and mount negotiation
typedef struct {
    const char* name;
    vfs_backend_type_t backend_type;
    vfs_feature_caps_t features;
} vfs_driver_info_t;


// File operations function pointers (Linux inode_operations style)
typedef struct {
    int (*read)(vfs_node_t* node, uint64_t offset, void* buffer, size_t size);
    int (*write)(vfs_node_t* node, uint64_t offset, const void* buffer, size_t size);
    int (*open)(vfs_node_t* node, int flags);
    int (*close)(vfs_node_t* node);
    struct dirent* (*readdir)(vfs_node_t* node, uint32_t index);
    vfs_node_t* (*finddir)(vfs_node_t* node, const char* name);
    int (*ioctl)(vfs_node_t* node, int request, void* arg);
} vfs_operations_t;

struct vfs_node {
    char name[256];
    uint32_t flags; // Type (file, dir, block device, pipe)
    uint32_t permissions; // Capability-based security ACLs
    uint32_t uid;
    uint32_t gid;
    uint64_t size; // File size in bytes
    uint64_t inode; // FS specific inode number
    vfs_backend_type_t backend_type; // Filesystem, block, blob, pseudo
    
    // Function table routing calls to the specific file system implementation
    vfs_operations_t* ops;
    
    // Pointer to specific FS data (e.g., ext4_inode, bfs_node)
    void* fs_data;
};

// Global VFS Root mounting point
extern vfs_node_t* vfs_root;

// Mount a specific filesystem driver to a path in the VFS tree
int vfs_mount(const char* target_path, vfs_node_t* fs_root);

// Standard open/read/write wrappers abstracting away the vfs lookup
int vfs_open(const char* path, int flags);
int vfs_read(int fd, void* buffer, size_t size);
int vfs_write(int fd, const void* buffer, size_t size);

// Register a filesystem/storage driver with the VFS core.
int vfs_register_driver(const vfs_driver_info_t* info);

// Lookup a registered driver descriptor by name.
const vfs_driver_info_t* vfs_get_driver(const char* name);

#ifdef TESTING
void vfs_test_reset_state(void);
#endif

#endif // BHARAT_VFS_H
