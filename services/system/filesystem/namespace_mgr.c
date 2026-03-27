#include "fs/namespace.h"
#include <stddef.h>

// Migrate logic from kernel/src/fs/namespace.c

int vfs_namespace_create(vfs_namespace_t* parent, vfs_namespace_t** out_namespace, capability_t* cap) {
    if (!out_namespace || !cap) return -1;
    if (parent && parent->sandbox_cap.capability_id != 0) {
        if ((cap->rights_mask & parent->sandbox_cap.rights_mask) != cap->rights_mask) return -2;
        if (cap->target_object_id != parent->sandbox_cap.target_object_id) return -3;
    }
    *out_namespace = NULL;
    return -4;
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
#include "fs/path.h"
int vfs_validate_path_rights(const char* path, uint32_t required_rights, capability_t* cap) {
    (void)required_rights;
    if (!path || !cap) return -1;
    return -1;
}
