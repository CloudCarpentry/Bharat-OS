#ifndef BHARAT_FS_NAMESPACE_H
#define BHARAT_FS_NAMESPACE_H

#include "fs/vfs.h"
#include "fs/mount.h"
#include "advanced/formal_verif.h"

/*
 * vfs_namespace_t: A per-process or per-sandbox view of the mount graph.
 * Provides a subset of available mounts based on the sandbox capabilities.
 */
typedef struct vfs_namespace {
    // The sandbox capabilities mapping to visible mounts and allowed paths
    capability_t sandbox_cap;

    // The list of mounts specifically visible to this namespace
    vfs_mount_t* mounts;
    uint32_t mount_count;

    // A reference back to a parent namespace if derived (or NULL)
    struct vfs_namespace* parent;
} vfs_namespace_t;

// Create a new, derived namespace restricted by capabilities
int vfs_namespace_create(vfs_namespace_t* parent, vfs_namespace_t** out_namespace, capability_t* cap);

// Look up a node within a given namespace
vfs_node_t* vfs_namespace_lookup(vfs_namespace_t* ns, const char* path, capability_t* caller_cap);

#endif // BHARAT_FS_NAMESPACE_H