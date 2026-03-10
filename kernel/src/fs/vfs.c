#include "fs/vfs.h"

#include <stddef.h>
#include <stdint.h>

#define VFS_MAX_DRIVERS 16

vfs_node_t *vfs_root = NULL;

static vfs_driver_info_t g_driver_registry[VFS_MAX_DRIVERS];
static size_t g_driver_count = 0;

size_t vfs_strnlen(const char *s, size_t max_len) {
    size_t i = 0;
    if (!s) {
        return 0;
    }
    while (i < max_len && s[i] != '\0') {
        i++;
    }
    return i;
}

int vfs_path_prefix_match(const char *path, const char *prefix) {
    size_t i = 0;

    if (!path || !prefix) {
        return 0;
    }

    while (prefix[i] != '\0' && path[i] != '\0') {
        if (prefix[i] != path[i]) {
            return 0;
        }
        i++;
    }

    if (prefix[i] != '\0') {
        return 0;
    }

    return (path[i] == '\0' || path[i] == '/');
}

static int vfs_streq(const char *a, const char *b) {
    size_t i = 0;
    if (!a || !b) {
        return 0;
    }
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return 0;
        }
        i++;
    }
    return a[i] == b[i];
}

int vfs_register_driver(const vfs_driver_info_t *info) {
    if (!info || !info->name || info->name[0] == '\0') {
        return -1;
    }

    if (g_driver_count >= VFS_MAX_DRIVERS) {
        return -1;
    }

    g_driver_registry[g_driver_count] = *info;
    g_driver_count++;
    return 0;
}

const vfs_driver_info_t *vfs_get_driver(const char *name) {
    size_t i;
    if (!name) {
        return NULL;
    }

    for (i = 0; i < g_driver_count; ++i) {
        if (vfs_streq(g_driver_registry[i].name, name)) {
            return &g_driver_registry[i];
        }
    }

    return NULL;
}

#ifdef TESTING
#include "fs/mount.h"
#include "fs/file.h"

void vfs_test_reset_state(void) {
    vfs_root = NULL;
    g_driver_count = 0;
    vfs_mount_test_reset_state();
    vfs_file_test_reset_state();
}
#endif
