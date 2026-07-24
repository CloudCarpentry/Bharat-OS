#ifndef BHARAT_VNODE_H
#define BHARAT_VNODE_H

#include <stdint.h>
#include <stddef.h>
#include "fs/fs_caps.h"
#include "../../staging/formal/formal_verif.h"

// Forward declarations to break cyclic dependencies
typedef struct vfs_file vfs_file_t;
typedef struct vfs_operations vfs_operations_t;
struct fs_superblock;

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
    int backend_type; // Use int directly here to prevent cycles

    // Back pointer to the filesystem instance
    struct fs_superblock* sb;

    // The mount context this vnode was resolved through
    struct vfs_mount* mnt_context;

    // Reference count and state flags
    uint32_t refcount;

    vfs_operations_t* ops;
    void* fs_data;
};

#endif // BHARAT_VNODE_H
