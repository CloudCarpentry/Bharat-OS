#ifndef BHARAT_VFS_H
#define BHARAT_VFS_H

#include <stdint.h>
#include <stddef.h>
#include "../../staging/formal/formal_verif.h" // For capability_t
#include "fs/vnode.h"
#include "fs/fs_caps.h"

/*
 * Bharat-OS Virtual File System (VFS)
 * Provides a unified, personality-neutral namespace for storage objects.
 * Features capability-based security, profile-driven backends, and
 * abstraction over block, blob, and filesystem resources.
 */

// VFS Namespace Object ID for mount authority
#define VFS_NAMESPACE_OBJECT_ID 0xFFFFFFFF

// Basic neutral flags
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

// Mount feature capabilities (Feature set, NOT security tokens)
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

typedef struct vfs_node vfs_node_t;
typedef struct vfs_mount vfs_mount_t;
typedef struct vfs_file vfs_file_t;

struct dirent {
    uint32_t d_ino;
    char d_name[256];
};

// Personality-neutral file operations
typedef struct vfs_operations {
    // Mount operations
    int (*mount)(vfs_mount_t* mnt, vfs_node_t* dev_node);
    int (*unmount)(vfs_mount_t* mnt);

    // Node operations
    vfs_node_t* (*lookup)(vfs_node_t* dir, const char* name);
    int (*create)(vfs_node_t* dir, const char* name, int flags);
    int (*remove)(vfs_node_t* dir, const char* name);

    // File operations
    int (*open)(vfs_node_t* node, vfs_file_t* file, int flags);
    int (*close)(vfs_file_t* file);
    int (*read)(vfs_file_t* file, uint64_t offset, void* buffer, size_t size);
    int (*write)(vfs_file_t* file, uint64_t offset, const void* buffer, size_t size);

    // Dir operations
    struct dirent* (*readdir)(vfs_file_t* file, uint32_t index);
    vfs_node_t* (*finddir)(vfs_node_t* node, const char* name); // legacy

    // Meta operations
    int (*ioctl)(vfs_file_t* file, int request, void* arg);
    int (*getattr)(vfs_node_t* node, void* stat_buf);
    int (*setattr)(vfs_node_t* node, void* stat_buf);
    int (*stat)(vfs_node_t* node, void* stat_buf); // legacy

    // Advanced
    int (*sync)(vfs_node_t* node);
    int (*mmap)(vfs_file_t* file, void** addr, size_t length, int prot, int flags, uint64_t offset); // hook if supported
    int (*watch)(vfs_node_t* node, void* watch_spec); // watch/notify hook optional
} vfs_operations_t;


// Global VFS Root mounting point (Legacy fallback or root namespace reference)
extern vfs_node_t* vfs_root;

// Register a filesystem/storage driver with the VFS core.
int vfs_register_driver(const vfs_driver_info_t* info);

// Lookup a registered driver descriptor by name.
const vfs_driver_info_t* vfs_get_driver(const char* name);

// Path utilities
int vfs_path_prefix_match(const char *path, const char *prefix);
size_t vfs_strnlen(const char *s, size_t max_len);

// Compatibility wrappers (non-capability versions)
int vfs_mount(const char* target_path, vfs_node_t* fs_root);
int vfs_open(const char* path, int flags);
int vfs_read(int fd, void* buffer, size_t size);
int vfs_write(int fd, const void* buffer, size_t size);
int vfs_close(int fd);

#ifdef TESTING
void vfs_test_reset_state(void);
#endif

#endif // BHARAT_VFS_H
