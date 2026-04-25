#include "fs/namespace.h"
#include <stddef.h>

// Migrate logic from kernel/src/fs/namespace.c

#include "mm.h"
#include "slab.h"
#include "lib/base/string.h"

int vfs_namespace_create(vfs_namespace_t* parent, vfs_namespace_t** out_namespace, capability_t* cap) {
    if (!out_namespace || !cap) return -1;

    /* Enforce deny-by-default: if a parent exists, the new namespace must be a subset of it */
    if (parent && parent->sandbox_cap.capability_id != 0) {
        if ((cap->rights_mask & parent->sandbox_cap.rights_mask) != cap->rights_mask) return -2;
        // In a production system, we'd also verify object IDs or path-based scope
    }

    vfs_namespace_t* ns = (vfs_namespace_t*)kmalloc(sizeof(vfs_namespace_t));
    if (!ns) return -3;

    memset(ns, 0, sizeof(vfs_namespace_t));
    ns->sandbox_cap = *cap;
    ns->parent = parent;

    /* Initially, a new namespace has no mounts. They must be explicitly added or inherited. */
    if (parent) {
        ns->root_dir = parent->root_dir;
        ns->cwd = parent->cwd;
    }

    *out_namespace = ns;
    return 0;
}

vfs_node_t* vfs_namespace_lookup(vfs_namespace_t* ns, const char* path, capability_t* caller_cap) {
    size_t i;
    vfs_node_t *best_node = NULL;
    size_t best_len = 0;

    if (!ns || !path || !caller_cap) return NULL;
    if (caller_cap->capability_id != 0 && ns->sandbox_cap.capability_id != 0) {
        if (caller_cap->target_object_id != ns->sandbox_cap.target_object_id) return NULL;
    }

    for (i = 0; i < ns->mount_count; ++i) {
        const char *mount_path = ns->mounts[i].target_path;
        if (caller_cap->capability_id != 0) {
            if (caller_cap->target_object_id != VFS_NAMESPACE_OBJECT_ID &&
                caller_cap->target_object_id != ns->mounts[i].root_node->object_id) continue;
        }

        if (!vfs_path_prefix_match(path, mount_path)) continue;

        size_t mount_len = ns->mounts[i].target_path_len;
        if (mount_len >= best_len) {
            best_len = mount_len;
            best_node = ns->mounts[i].root_node;
        }
    }
    return best_node;
}

// from kernel/src/fs/path.c
int vfs_validate_path_rights(const char* path, uint32_t required_rights, capability_t* cap) {
    if (!path || !cap) return -1;

    /* Reject any path with traversal (..) to prevent sandbox breakout.
     * We must check if ".." is a path component. */
    const char *ptr = path;
    while (*ptr) {
        // Check for ".." at the beginning or after a "/"
        if (ptr == path || ptr[-1] == '/') {
            if (ptr[0] == '.' && ptr[1] == '.' && (ptr[2] == '/' || ptr[2] == '\0')) {
                return -3; // Traversal detected
            }
        }
        ptr++;
    }

    /* In a production system, we'd also check if the path matches the capability's allowed prefix */
    if (cap->capability_id != 0) {
        if ((cap->rights_mask & required_rights) != required_rights) {
            return -2;
        }
    }
    return 0;
}
