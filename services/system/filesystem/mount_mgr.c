#include <stdint.h>
#include <stddef.h>
#include "fs/vfs.h"
#include "fs/mount.h"

// Migrate logic from kernel/src/fs/mount.c

#define VFS_MAX_MOUNTS 16

static vfs_mount_t g_mounts[VFS_MAX_MOUNTS];
static size_t g_mount_count = 0;
static uint32_t g_mount_lock = 0;

static void vfs_strcpy(char *dst, const char *src, size_t dst_size) {
    size_t i = 0;
    if (!dst || dst_size == 0) {
        return;
    }
    if (!src) {
        dst[0] = '\0';
        return;
    }
    while (i + 1 < dst_size && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

int vfs_mount_fs(const char* target_path, vfs_node_t* fs_root, capability_t* caller_cap) {
    if (!target_path || !fs_root || !caller_cap) {
        return -1; // Invalid Arg
    }

    if (caller_cap->target_object_id != VFS_NAMESPACE_OBJECT_ID && caller_cap->capability_id != 0) {
        return -2; // Cap Denied
    }
    if ((caller_cap->rights_mask & 2) != 2 && caller_cap->capability_id != 0) {
        return -2; // Cap Denied
    }

    if (target_path[0] != '/') {
        return -1; // Invalid Arg
    }

    while (__atomic_test_and_set(&g_mount_lock, __ATOMIC_ACQUIRE)) {
        // Spin
    }

    if (g_mount_count >= VFS_MAX_MOUNTS) {
        __atomic_clear(&g_mount_lock, __ATOMIC_RELEASE);
        return -3; // No Memory
    }

    vfs_strcpy(g_mounts[g_mount_count].target_path, target_path, sizeof(g_mounts[g_mount_count].target_path));
    g_mounts[g_mount_count].target_path_len = vfs_strnlen(target_path, sizeof(g_mounts[g_mount_count].target_path));
    g_mounts[g_mount_count].root_node = fs_root;

    if (vfs_path_prefix_match(target_path, "/")) {
        vfs_root = fs_root;
    }

    g_mount_count++;
    __atomic_clear(&g_mount_lock, __ATOMIC_RELEASE);
    return 0; // OK
}

int vfs_mount(const char* target_path, vfs_node_t* fs_root) {
    capability_t dummy_cap = {0};
    return vfs_mount_fs(target_path, fs_root, &dummy_cap);
}

vfs_node_t* vfs_resolve_mount_path(const char* path, capability_t* caller_cap) {
    size_t best_len = 0;
    vfs_node_t *best_node = NULL;
    size_t i = 0;

    if (!path || !caller_cap) {
        return NULL;
    }

    while (__atomic_test_and_set(&g_mount_lock, __ATOMIC_ACQUIRE)) {
        // Spin
    }

    for (i = 0; i < g_mount_count; ++i) {
        const char *mount_path = g_mounts[i].target_path;

        if (caller_cap->capability_id != 0) {
            if (caller_cap->target_object_id != VFS_NAMESPACE_OBJECT_ID &&
                caller_cap->target_object_id != g_mounts[i].root_node->object_id) {
                continue;
            }
        }

        if (!vfs_path_prefix_match(path, mount_path)) {
            continue;
        }

        size_t mount_len = g_mounts[i].target_path_len;

        if (mount_len >= best_len) {
            best_len = mount_len;
            best_node = g_mounts[i].root_node;
        }
    }

    __atomic_clear(&g_mount_lock, __ATOMIC_RELEASE);

    if (!best_node && vfs_root) {
        if (caller_cap->capability_id == 0 ||
            caller_cap->target_object_id == VFS_NAMESPACE_OBJECT_ID ||
            caller_cap->target_object_id == vfs_root->object_id) {
            best_node = vfs_root;
        }
    }

    return best_node;
}
