#include "fs/vfs.h"
#include <stddef.h>

#define VFS_MAX_DRIVERS 16

vfs_node_t *vfs_root = NULL;

static vfs_driver_info_t g_driver_registry[VFS_MAX_DRIVERS];
static size_t g_driver_count = 0;
static uint32_t g_driver_lock = 0;

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

static int vfs_streq(const char *a, const char *b) {
    size_t i = 0;
    if (!a || !b) return 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) return 0;
        i++;
    }
    return a[i] == b[i];
}

int vfs_register_driver(const vfs_driver_info_t *info) {
    if (!info || !info->name || info->name[0] == '\0') return -1;
    while (__atomic_test_and_set(&g_driver_lock, __ATOMIC_ACQUIRE)) {}
    if (g_driver_count >= VFS_MAX_DRIVERS) {
        __atomic_clear(&g_driver_lock, __ATOMIC_RELEASE);
        return -1;
    }
    g_driver_registry[g_driver_count++] = *info;
    __atomic_clear(&g_driver_lock, __ATOMIC_RELEASE);
    return 0;
}

const vfs_driver_info_t *vfs_get_driver(const char *name) {
    const vfs_driver_info_t *found = NULL;
    if (!name) return NULL;
    while (__atomic_test_and_set(&g_driver_lock, __ATOMIC_ACQUIRE)) {}
    for (size_t i = 0; i < g_driver_count; ++i) {
        if (vfs_streq(g_driver_registry[i].name, name)) {
            found = &g_driver_registry[i];
            break;
        }
    }
    __atomic_clear(&g_driver_lock, __ATOMIC_RELEASE);
    return found;
}

#ifdef TESTING
void vfs_test_reset_state(void) {
    vfs_root = NULL;
    g_driver_count = 0;
    g_driver_lock = 0;
}
#endif
