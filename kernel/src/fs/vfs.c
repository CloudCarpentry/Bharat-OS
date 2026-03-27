#include "fs/vfs.h"
#include "kernel/status.h"

// PHASE1_COMPAT_SHIM: VFS policy logic has moved to services/system/filesystem/.
// These are minimal compatibility shims to keep the kernel building.
// They will eventually forward to the FS service via IPC/uRPC.

vfs_node_t *vfs_root = NULL;

size_t vfs_strnlen(const char *s, size_t max_len) {
    size_t i = 0;
    if (!s) return 0;
    while (i < max_len && s[i] != '\0') i++;
    return i;
}

int vfs_path_prefix_match(const char *path, const char *prefix) {
    size_t i = 0;
    if (!path || !prefix) return 0;
    while (prefix[i] != '\0' && path[i] != '\0') {
        if (prefix[i] != path[i]) return 0;
        i++;
    }
    if (prefix[i] != '\0') return 0;
    return (path[i] == '\0' || path[i] == '/');
}

int vfs_register_driver(const vfs_driver_info_t *info) {
    // Should be registered directly with the filesystem service via uRPC.
    (void)info;
    return K_ERR_REQUIRES_FS_SERVICE;
}

const vfs_driver_info_t *vfs_get_driver(const char *name) {
    (void)name;
    return NULL; // Requires FS service
}

#ifdef TESTING
void vfs_test_reset_state(void) {
    vfs_root = NULL;
}
#endif
