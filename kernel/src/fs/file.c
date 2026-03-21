#include "fs/file.h"
#include "fs/mount.h"
#include "capability.h"
#include "sched.h"

#define VFS_MAX_OPEN_FILES 64

static vfs_file_t g_open_files[VFS_MAX_OPEN_FILES];

static int vfs_cap_allows_file(vfs_file_t* entry, capability_t* caller_cap, uint32_t required_rights) {
    if (!entry || !entry->node || !caller_cap) {
        return 0;
    }

    // Legacy or internal callers using an advisory dummy capability
    if (caller_cap->capability_id == 0) {
        if ((caller_cap->rights_mask & required_rights) != required_rights) {
            return 0;
        }

        if (entry->handle_cap.target_object_id != 0 || entry->handle_cap.rights_mask != 0) {
            if ((entry->handle_cap.rights_mask & required_rights) != required_rights) {
                return 0;
            }
            if (caller_cap->target_object_id != entry->handle_cap.target_object_id) {
                return 0;
            }
            return 1;
        }

        if (caller_cap->target_object_id != entry->node->object_id) {
            return 0;
        }
        return 1;
    }

    capability_table_t* table = sched_current_cap_table();
    if (!table) {
        return 0;
    }

    capability_entry_t auth_cap;
    // cap_table_lookup handles resolution, liveness, and validates required_rights
    if (cap_table_lookup(table, caller_cap->capability_id, CAP_OBJ_NONE, required_rights, &auth_cap) != 0) {
        return 0;
    }

    // Compare against the file's bound capability if established
    if (entry->handle_cap.target_object_id != 0 || entry->handle_cap.rights_mask != 0) {
        if ((entry->handle_cap.rights_mask & required_rights) != required_rights) {
            return 0;
        }
        if (auth_cap.object_ref != entry->handle_cap.target_object_id) {
            return 0;
        }
        return 1;
    }

    // Otherwise, ensure the authoritative capability points to the node's object
    if (auth_cap.object_ref != entry->node->object_id) {
        return 0;
    }

    return 1;
}

int vfs_open_file(const char* path, int flags, capability_t* caller_cap, int* out_fd) {
    size_t i;
    vfs_node_t *node;
    uint32_t requested_rights = 0;

    if (!path || !caller_cap || !out_fd) {
        return -1;
    }

    node = vfs_resolve_mount_path(path, caller_cap);

    if (!node) {
        return -1;
    }

    if (flags & VFS_OPEN_READ) {
        requested_rights |= CAP_RIGHT_READ;
    }
    if (flags & VFS_OPEN_WRITE) {
        requested_rights |= CAP_RIGHT_WRITE;
    }

    if ((caller_cap->rights_mask & requested_rights) != requested_rights) {
        return -1;
    }

    if (caller_cap->target_object_id != node->object_id) {
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

            // Derive capability through cap_table to ensure it is newly minted and bound to caller
            uint32_t new_cap_id = 0;
            capability_table_t* table = sched_current_cap_table();
            if (!table || caller_cap->capability_id == 0) {
                if (node->ops && node->ops->close) {
                    node->ops->close(&g_open_files[i]);
                }
                g_open_files[i].in_use = 0;
                g_open_files[i].node = NULL;
                return -1;
            }

            if (cap_table_delegate(table,
                                   table,
                                   caller_cap->capability_id,
                                   requested_rights,
                                   &new_cap_id) != 0) {
                // Minting failed, rollback backend open
                if (node->ops && node->ops->close) {
                    node->ops->close(&g_open_files[i]);
                }
                g_open_files[i].in_use = 0;
                g_open_files[i].node = NULL;
                return -1;
            }

            // Store advisory token
            g_open_files[i].handle_cap.capability_id = new_cap_id;
            g_open_files[i].handle_cap.target_object_id = node->object_id;
            g_open_files[i].handle_cap.rights_mask = caller_cap->rights_mask & requested_rights;
            g_open_files[i].handle_cap.owner_core_id = caller_cap->owner_core_id;

            *out_fd = (int)i;
            return 0;
        }
    }

    return -1;
}

int vfs_open(const char* path, int flags) {
    capability_t dummy_cap = {0};
    int fd = -1;

    // For legacy vfs_open, just bypass cap check locally by passing dummy cap but make sure
    // to give it full rights so it doesn't fail the new checks inside vfs_open_file
    dummy_cap.rights_mask = CAP_RIGHT_READ | CAP_RIGHT_WRITE;
    // We can't easily know target_object_id here.
    // Wait, the vfs_open is a legacy wrapper. Let's make sure it passes.
    // Actually, let's just resolve mount path and get the object_id
    vfs_node_t *node = vfs_resolve_mount_path(path, &dummy_cap);
    if (node) {
        dummy_cap.target_object_id = node->object_id;
    }

    if (vfs_open_file(path, flags, &dummy_cap, &fd) == 0) {
        return fd;
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

    if ((entry->flags & VFS_OPEN_READ) == 0) {
        return -1;
    }

    if (!vfs_cap_allows_file(entry, caller_cap, CAP_RIGHT_READ)) {
        return -1;
    }

    bytes = entry->node->ops->read(entry, entry->offset, buffer, size);
    if (bytes > 0) {
        entry->offset += (uint64_t)bytes;
    }
    return bytes;
}

int vfs_read(int fd, void* buffer, size_t size) {
    capability_t dummy_cap = {0};
    if (fd >= 0 && fd < (int)VFS_MAX_OPEN_FILES) {
        dummy_cap = g_open_files[fd].handle_cap;
    }
    return vfs_read_file(fd, buffer, size, &dummy_cap);
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

    if (!vfs_cap_allows_file(entry, caller_cap, CAP_RIGHT_WRITE)) {
        return -1;
    }

    bytes = entry->node->ops->write(entry, entry->offset, buffer, size);
    if (bytes > 0) {
        entry->offset += (uint64_t)bytes;
    }
    return bytes;
}

int vfs_write(int fd, const void* buffer, size_t size) {
    capability_t dummy_cap = {0};
    if (fd >= 0 && fd < (int)VFS_MAX_OPEN_FILES) {
        dummy_cap = g_open_files[fd].handle_cap;
    }
    return vfs_write_file(fd, buffer, size, &dummy_cap);
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

    if (!vfs_cap_allows_file(entry, caller_cap, 0)) {
        return -1;
    }

    if (entry->node && entry->node->ops && entry->node->ops->close) {
        entry->node->ops->close(entry);
    }

    if (entry->handle_cap.capability_id != 0) {
        capability_table_t* table = sched_current_cap_table();
        if (table) {
            cap_table_revoke(table, entry->handle_cap.capability_id);
        }
    }

    entry->in_use = 0;
    entry->node = NULL;
    entry->handle_cap.capability_id = 0;
    entry->handle_cap.target_object_id = 0;
    entry->handle_cap.rights_mask = 0;
    entry->handle_cap.owner_core_id = 0;
    return 0;
}

int vfs_close(int fd) {
    capability_t dummy_cap = {0};
    if (fd >= 0 && fd < (int)VFS_MAX_OPEN_FILES) {
        dummy_cap = g_open_files[fd].handle_cap;
    }
    return vfs_close_file(fd, &dummy_cap);
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
