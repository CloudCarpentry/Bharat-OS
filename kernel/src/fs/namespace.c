#include "fs/namespace.h"
#include "kernel/status.h"

int vfs_namespace_create(vfs_namespace_t* parent, vfs_namespace_t** out_namespace, capability_t* cap) {
    if (!out_namespace || !cap) {
        return K_ERR_INVALID_ARG;
    }

    // Verify cap allows namespace derivation from parent
    if (parent && parent->sandbox_cap.capability_id != 0) {
        // Child cannot have more rights than the parent
        if ((cap->rights_mask & parent->sandbox_cap.rights_mask) != cap->rights_mask) {
            return K_ERR_CAP_DENIED;
        }

        // Ensure cap object matches the namespace boundary
        if (cap->target_object_id != parent->sandbox_cap.target_object_id) {
            return K_ERR_CAP_WRONG_TYPE;
        }
    }

    // In a full implementation, allocate and derive a restricted view
    // from the parent namespace.
    *out_namespace = NULL;

    return K_ERR_UNSUPPORTED; // Unimplemented
}

vfs_node_t* vfs_namespace_lookup(vfs_namespace_t* ns, const char* path, capability_t* caller_cap) {
    size_t i;
    vfs_node_t *best_node = NULL;
    size_t best_len = 0;

    if (!ns || !path || !caller_cap) {
        return NULL;
    }

    // Enforce sandbox capability:
    // Ensure caller has access to the namespace
    if (caller_cap->capability_id != 0 && ns->sandbox_cap.capability_id != 0) {
        if (caller_cap->target_object_id != ns->sandbox_cap.target_object_id) {
            return NULL;
        }
    }

    // Resolve path relative to the specific namespace's mount list
    for (i = 0; i < ns->mount_count; ++i) {
        const char *mount_path = ns->mounts[i].target_path;

        // Capability filter
        if (caller_cap->capability_id != 0) {
            if (caller_cap->target_object_id != VFS_NAMESPACE_OBJECT_ID &&
                caller_cap->target_object_id != ns->mounts[i].root_node->object_id) {
                continue;
            }
        }

        if (!vfs_path_prefix_match(path, mount_path)) {
            continue;
        }

        size_t mount_len = ns->mounts[i].target_path_len;

        if (mount_len >= best_len) {
            best_len = mount_len;
            best_node = ns->mounts[i].root_node;
        }
    }

    // If no mount matches within the namespace, the path doesn't exist here
    return best_node;
}
