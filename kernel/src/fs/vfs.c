#include "fs/vfs.h"

#include <stddef.h>
#include <stdint.h>

#define VFS_MAX_DRIVERS 16
#define VFS_MAX_MOUNTS 16
#define VFS_MAX_OPEN_FILES 64

#ifndef VFS_OPEN_READ
#define VFS_OPEN_READ 0x1
#endif
#ifndef VFS_OPEN_WRITE
#define VFS_OPEN_WRITE 0x2
#endif

typedef struct {
    char target_path[256];
    vfs_node_t *root;
} vfs_mount_entry_t;

typedef struct {
    int in_use;
    int flags;
    uint64_t offset;
    vfs_node_t *node;
} vfs_fd_entry_t;

vfs_node_t *vfs_root = NULL;

static vfs_driver_info_t g_driver_registry[VFS_MAX_DRIVERS];
static size_t g_driver_count = 0;

static vfs_mount_entry_t g_mounts[VFS_MAX_MOUNTS];
static size_t g_mount_count = 0;

static vfs_fd_entry_t g_open_files[VFS_MAX_OPEN_FILES];

static size_t vfs_strnlen(const char *s, size_t max_len) {
    size_t i = 0;
    if (!s) {
        return 0;
    }
    while (i < max_len && s[i] != '\0') {
        i++;
    }
    return i;
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

static int path_prefix_match(const char *path, const char *prefix) {
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

static vfs_node_t *vfs_resolve_path(const char *path) {
    size_t best_len = 0;
    vfs_node_t *best_node = NULL;
    size_t i = 0;

    if (!path) {
        return NULL;
    }

    for (i = 0; i < g_mount_count; ++i) {
        const char *mount_path = g_mounts[i].target_path;
        if (!path_prefix_match(path, mount_path)) {
            continue;
        }

        size_t mount_len = vfs_strnlen(mount_path, sizeof(g_mounts[i].target_path));
        if (mount_len >= best_len) {
            best_len = mount_len;
            best_node = g_mounts[i].root;
        }
    }

    if (!best_node && vfs_root) {
        best_node = vfs_root;
    }

    return best_node;
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

int vfs_mount(const char *target_path, vfs_node_t *fs_root) {
    if (!target_path || !fs_root) {
        return -1;
    }

    if (g_mount_count >= VFS_MAX_MOUNTS) {
        return -1;
    }

    if (target_path[0] != '/') {
        return -1;
    }

    vfs_strcpy(g_mounts[g_mount_count].target_path, target_path, sizeof(g_mounts[g_mount_count].target_path));
    g_mounts[g_mount_count].root = fs_root;

    if (vfs_streq(target_path, "/")) {
        vfs_root = fs_root;
    }

    g_mount_count++;
    return 0;
}

int vfs_open(const char *path, int flags) {
    size_t i;
    vfs_node_t *node = vfs_resolve_path(path);

    if (!node) {
        return -1;
    }

    if (node->backend_type == VFS_BACKEND_BLOB && (flags & VFS_OPEN_WRITE)) {
        return -1;
    }

    if (node->ops && node->ops->open) {
        if (node->ops->open(node, flags) != 0) {
            return -1;
        }
    }

    for (i = 0; i < VFS_MAX_OPEN_FILES; ++i) {
        if (!g_open_files[i].in_use) {
            g_open_files[i].in_use = 1;
            g_open_files[i].flags = flags;
            g_open_files[i].offset = 0;
            g_open_files[i].node = node;
            return (int)i;
        }
    }

    return -1;
}

int vfs_read(int fd, void *buffer, size_t size) {
    int bytes;
    vfs_fd_entry_t *entry;

    if (fd < 0 || fd >= (int)VFS_MAX_OPEN_FILES) {
        return -1;
    }

    entry = &g_open_files[fd];
    if (!entry->in_use || !entry->node || !entry->node->ops || !entry->node->ops->read) {
        return -1;
    }

    bytes = entry->node->ops->read(entry->node, entry->offset, buffer, size);
    if (bytes > 0) {
        entry->offset += (uint64_t)bytes;
    }
    return bytes;
}

int vfs_write(int fd, const void *buffer, size_t size) {
    int bytes;
    vfs_fd_entry_t *entry;

    if (fd < 0 || fd >= (int)VFS_MAX_OPEN_FILES) {
        return -1;
    }

    entry = &g_open_files[fd];
    if (!entry->in_use || !entry->node || !entry->node->ops || !entry->node->ops->write) {
        return -1;
    }

    if ((entry->flags & VFS_OPEN_WRITE) == 0) {
        return -1;
    }

    if (entry->node->backend_type == VFS_BACKEND_BLOB) {
        return -1;
    }

    bytes = entry->node->ops->write(entry->node, entry->offset, buffer, size);
    if (bytes > 0) {
        entry->offset += (uint64_t)bytes;
    }
    return bytes;
}

#ifdef TESTING
void vfs_test_reset_state(void) {
    size_t i;

    vfs_root = NULL;
    g_driver_count = 0;
    g_mount_count = 0;

    for (i = 0; i < VFS_MAX_OPEN_FILES; ++i) {
        g_open_files[i].in_use = 0;
        g_open_files[i].flags = 0;
        g_open_files[i].offset = 0;
        g_open_files[i].node = NULL;
    }
}
#endif
