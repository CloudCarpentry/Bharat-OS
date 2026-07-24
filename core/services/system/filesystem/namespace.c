#include "fs/namespace.h"
#include "kernel/status.h"

// PHASE1_COMPAT_SHIM: Namespace derivation logic moved to services/system/filesystem/.

int vfs_namespace_create(vfs_namespace_t* parent, vfs_namespace_t** out_namespace, capability_t* cap) {
    (void)parent;
    (void)cap;
    if (out_namespace) *out_namespace = NULL;
    return K_ERR_REQUIRES_FS_SERVICE;
}

vfs_node_t* vfs_namespace_lookup(vfs_namespace_t* ns, const char* path, capability_t* caller_cap) {
    (void)ns;
    (void)path;
    (void)caller_cap;
    return NULL;
}
