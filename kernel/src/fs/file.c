#include "fs/file.h"
#include "fs/mount.h"

#define VFS_MAX_OPEN_FILES 64

static vfs_file_t g_open_files[VFS_MAX_OPEN_FILES];

static int vfs_cap_allows(const vfs_file_t *entry, const capability_t *caller_cap, uint32_t required_right) {
    if (!entry || !entry->node || !caller_cap) {
        return 0;
    }

    // Must have the required right (if one is specified)
    if (required_right != 0 && (caller_cap->rights_mask & required_right) == 0) {
        return 0;
    }

    // Must be bound to the exact same object
    if (caller_cap->target_object_id != entry->node->object_id) {
        return 0;
    }

    // If handle_cap was properly initialized at open-time, check it
    // Right now it's a TODO in vfs_open_file, but if it is populated (e.g., capability_id != 0),
    // ensure caller_cap matches the handle's cap.
    if (entry->handle_cap.capability_id != 0) {
        if (caller_cap->capability_id != entry->handle_cap.capability_id) {
            return 0;
        }
    }

    return 1;
}

int vfs_open_file(const char* path, int flags, capability_t* caller_cap, int* out_fd) {
    size_t i;
    vfs_node_t *node;

    if (!path || !caller_cap || !out_fd) {
        return -1;
    }

    node = vfs_resolve_mount_path(path, caller_cap);

    if (!node) {
        return -1;
    }

    if (node->backend_type == VFS_BACKEND_BLOB && (flags & VFS_OPEN_WRITE)) {
        return -1;
    }

    for (i = 0; i < VFS_MAX_OPEN_FILES; ++i) {
        if (!g_open_files[i].in_use) {

            g_open_files[i].in_use = 1;
            g_open_files[i].flags = flags;
            g_open_files[i].offset = 0;
            g_open_files[i].node = node;

            // Allow underlying implementation to populate/reject handle
            if (node->ops && node->ops->open) {
                if (node->ops->open(node, &g_open_files[i], flags) != 0) {
                    g_open_files[i].in_use = 0;
                    g_open_files[i].node = NULL;
                    return -1;
                }
            }

            // TODO: Generate handle_cap based on path/node cap and flags

            *out_fd = (int)i;
            return 0;
        }
    }

    return -1;
}

int vfs_read_file(int fd, void* buffer, size_t size, capability_t* caller_cap) {
    int bytes;
    vfs_file_t *entry;

    if (fd < 0 || fd >= (int)VFS_MAX_OPEN_FILES || !caller_cap) {
        return -1;
    }

    entry = &g_open_files[fd];
    if (!entry->in_use || !entry->node || !entry->node->ops || !entry->node->ops->read) {
        return -1;
    }

    if (!vfs_cap_allows(entry, caller_cap, CAP_RIGHT_READ)) {
        return -1;
    }

    bytes = entry->node->ops->read(entry, entry->offset, buffer, size);
    if (bytes > 0) {
        entry->offset += (uint64_t)bytes;
    }
    return bytes;
}

int vfs_write_file(int fd, const void* buffer, size_t size, capability_t* caller_cap) {
    int bytes;
    vfs_file_t *entry;

    if (fd < 0 || fd >= (int)VFS_MAX_OPEN_FILES || !caller_cap) {
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

    if (!vfs_cap_allows(entry, caller_cap, CAP_RIGHT_WRITE)) {
        return -1;
    }

    bytes = entry->node->ops->write(entry, entry->offset, buffer, size);
    if (bytes > 0) {
        entry->offset += (uint64_t)bytes;
    }
    return bytes;
}

int vfs_close_file(int fd, capability_t* caller_cap) {
    vfs_file_t *entry;

    if (fd < 0 || fd >= (int)VFS_MAX_OPEN_FILES || !caller_cap) {
        return -1;
    }

    entry = &g_open_files[fd];
    if (!entry->in_use) {
        return -1;
    }

    if (!vfs_cap_allows(entry, caller_cap, 0)) {
        return -1;
    }

    if (entry->node && entry->node->ops && entry->node->ops->close) {
        entry->node->ops->close(entry);
    }

    entry->in_use = 0;
    entry->node = NULL;
    return 0;
}

#ifdef TESTING
void vfs_file_test_reset_state(void) {
    size_t i;
    for (i = 0; i < VFS_MAX_OPEN_FILES; ++i) {
        g_open_files[i].in_use = 0;
        g_open_files[i].flags = 0;
        g_open_files[i].offset = 0;
        g_open_files[i].node = NULL;
    }
}
#endif
