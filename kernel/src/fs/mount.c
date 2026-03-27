#include "fs/mount.h"
#include "kernel/status.h"

// PHASE1_COMPAT_SHIM: Mount policy logic has moved to services/system/filesystem/.
// These are minimal compatibility shims to keep the kernel building.
// They will eventually forward to the FS service via IPC/uRPC.

int vfs_mount_fs(const char* target_path, vfs_node_t* fs_root, capability_t* caller_cap) {
    (void)target_path;
    (void)fs_root;
    (void)caller_cap;
    // Real implementation requires IPC to FS Service
    return K_ERR_REQUIRES_FS_SERVICE;
}

int vfs_mount(const char* target_path, vfs_node_t* fs_root) {
    (void)target_path;
    (void)fs_root;
    return K_ERR_REQUIRES_FS_SERVICE;
}

vfs_node_t* vfs_resolve_mount_path(const char* path, capability_t* caller_cap) {
    (void)path;
    (void)caller_cap;
    // Resolving paths happens in userspace VFS
    return NULL;
}

#ifdef TESTING
void vfs_mount_test_reset_state(void) {
    // Stub
}
#endif
