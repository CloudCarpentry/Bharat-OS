#include "fs/mount.h"

#define VFS_MAX_MOUNTS 16

static vfs_mount_t g_mounts[VFS_MAX_MOUNTS];
static size_t g_mount_count = 0;

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
        return -1;
    }

    // TODO: Verify caller_cap allows mounting at target_path

    if (g_mount_count >= VFS_MAX_MOUNTS) {
        return -1;
    }

    if (target_path[0] != '/') {
        return -1;
    }

    vfs_strcpy(g_mounts[g_mount_count].target_path, target_path, sizeof(g_mounts[g_mount_count].target_path));
    g_mounts[g_mount_count].root_node = fs_root;

    if (vfs_path_prefix_match(target_path, "/")) {
        vfs_root = fs_root;
    }

    g_mount_count++;
    return 0;
}

vfs_node_t* vfs_resolve_mount_path(const char* path, capability_t* caller_cap) {
    size_t best_len = 0;
    vfs_node_t *best_node = NULL;
    size_t i = 0;

    if (!path || !caller_cap) {
        return NULL;
    }

    // TODO: Filter visible mounts based on caller_cap
    for (i = 0; i < g_mount_count; ++i) {
        const char *mount_path = g_mounts[i].target_path;
        if (!vfs_path_prefix_match(path, mount_path)) {
            continue;
        }

        size_t mount_len = 0;
        while(mount_path[mount_len] != '\0' && mount_len < sizeof(g_mounts[i].target_path)) {
            mount_len++;
        }

        if (mount_len >= best_len) {
            best_len = mount_len;
            best_node = g_mounts[i].root_node;
        }
    }

    if (!best_node && vfs_root) {
        best_node = vfs_root;
    }

    return best_node;
}

#ifdef TESTING
void vfs_mount_test_reset_state(void) {
    g_mount_count = 0;
}
#endif
