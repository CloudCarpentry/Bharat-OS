#ifndef BHARAT_FS_MOUNT_H
#define BHARAT_FS_MOUNT_H

#include "fs/vfs.h"
#include "../../staging/formal/formal_verif.h"
#include "fs/superblock.h"

/*
 * vfs_mount_t: Represents a mounted filesystem instance.
 * Governs namespace visibility, path-rooted rights, and storage-class rights
 * via a mount capability token.
 */
typedef struct vfs_mount {
    char target_path[256];      // The point in the VFS tree where this is mounted
    size_t target_path_len;     // Cached length of target_path
    vfs_node_t* root_node;      // The root node of the mounted filesystem
    const vfs_driver_info_t* driver; // The driver backing this mount

    // Mount-specific capability token (restricts path, class, TTL)
    capability_t mount_cap;

    // The underlying filesystem instance this mount exposes
    struct fs_superblock* sb;

    // Mount flags (e.g., read-only, no-exec)
    uint32_t mount_flags;

    // Provenance metadata
    uint32_t origin_id;         // System component or service ID that issued the mount
    uint64_t mount_time;        // Boot-relative timestamp of mount

    // References to next mount in system list or namespace mount graph
    struct vfs_mount* next;
} vfs_mount_t;

// Production-grade mount flags
#define VFS_MOUNT_READONLY (1u << 0)
#define VFS_MOUNT_NOEXEC   (1u << 1)
#define VFS_MOUNT_NODEV    (1u << 2)
#define VFS_MOUNT_NOSUID   (1u << 3)

// Capability-checked mount request
int vfs_mount_fs(const char* target_path, vfs_node_t* fs_root, capability_t* caller_cap);

// Resolve a path using mount capabilities
vfs_node_t* vfs_resolve_mount_path(const char* path, capability_t* caller_cap);

#ifdef TESTING
void vfs_mount_test_reset_state(void);
#endif

#endif // BHARAT_FS_MOUNT_H
