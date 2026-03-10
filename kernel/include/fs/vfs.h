#ifndef BHARAT_VFS_H
#define BHARAT_VFS_H

#include <stdint.h>
#include <stddef.h>
#include "advanced/formal_verif.h" // For capability_t

/*
 * Bharat-OS Virtual File System (VFS)
 * Provides a unified, personality-neutral namespace for storage objects.
 * Features capability-based security, profile-driven backends, and
 * abstraction over block, blob, and filesystem resources.
 */

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

// Personality-neutral file operations
typedef struct {
    int (*read)(vfs_file_t* file, uint64_t offset, void* buffer, size_t size);
    int (*write)(vfs_file_t* file, uint64_t offset, const void* buffer, size_t size);
    int (*open)(vfs_node_t* node, vfs_file_t* file, int flags);
    int (*close)(vfs_file_t* file);
    struct dirent* (*readdir)(vfs_file_t* file, uint32_t index);
    vfs_node_t* (*finddir)(vfs_node_t* node, const char* name);
    int (*ioctl)(vfs_file_t* file, int request, void* arg);
    int (*stat)(vfs_node_t* node, void* stat_buf); // Generic stat-like structure
} vfs_operations_t;

/*
 * vfs_node_t: The canonical storage object.
 * Represents a file, directory, or device. Does NOT contain ambient
 * permissions, but rather links to the capability system via provenance.
 */
struct vfs_node {
    char name[256];
    uint32_t flags; // Type (file, dir, block device, pipe)

    // Ownership/Security Metadata
    // The canonical object ID to which capabilities grant access
    uint32_t object_id;
    // Link to capability provenance/derivation tree
    capability_t* provenance_cap;

    uint64_t size; // File size in bytes
    uint64_t inode; // FS specific inode number
    vfs_backend_type_t backend_type;
    
    vfs_operations_t* ops;
    void* fs_data;
};

// Global VFS Root mounting point (Legacy fallback or root namespace reference)
extern vfs_node_t* vfs_root;

// Register a filesystem/storage driver with the VFS core.
int vfs_register_driver(const vfs_driver_info_t* info);

// Lookup a registered driver descriptor by name.
const vfs_driver_info_t* vfs_get_driver(const char* name);

// Path utilities
int vfs_path_prefix_match(const char *path, const char *prefix);
size_t vfs_strnlen(const char *s, size_t max_len);

#ifdef TESTING
void vfs_test_reset_state(void);
#endif

#endif // BHARAT_VFS_H
