#ifndef BHARAT_FS_MOUNT_H
#define BHARAT_FS_MOUNT_H

#include "fs/vfs.h"
#include "advanced/formal_verif.h"

/*
 * vfs_mount_t: Represents a mounted filesystem instance.
 * Governs namespace visibility, path-rooted rights, and storage-class rights
 * via a mount capability token.
 */
typedef struct vfs_mount {
    char target_path[256];      // The point in the VFS tree where this is mounted
    vfs_node_t* root_node;      // The root node of the mounted filesystem
    const vfs_driver_info_t* driver; // The driver backing this mount

    // Mount-specific capability token (restricts path, class, TTL)
    capability_t mount_cap;

    // References to next mount in system list or namespace mount graph
    struct vfs_mount* next;
} vfs_mount_t;

// Capability-checked mount request
int vfs_mount_fs(const char* target_path, vfs_node_t* fs_root, capability_t* caller_cap);

// Resolve a path using mount capabilities
vfs_node_t* vfs_resolve_mount_path(const char* path, capability_t* caller_cap);

#ifdef TESTING
void vfs_mount_test_reset_state(void);
#endif

#endif // BHARAT_FS_MOUNT_H