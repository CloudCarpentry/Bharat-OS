#include "fs/vfs.h"
#include "fs/file.h"
#include "fs/mount.h"
#include "capability.h"
#include "sched/sched.h"

// Migrate logic from kernel/src/fs/file.c

#define VFS_MAX_OPEN_FILES 64

static vfs_file_t g_open_files[VFS_MAX_OPEN_FILES];
static uint32_t g_open_files_lock = 0;

static int vfs_cap_allows_file(vfs_file_t* entry, capability_t* caller_cap, uint32_t required_rights) {
    if (!entry || !entry->node || !caller_cap) return 0;
    if (caller_cap->capability_id == 0) {
        if ((caller_cap->rights_mask & required_rights) != required_rights) return 0;
        if (entry->handle_cap.target_object_id != 0 || entry->handle_cap.rights_mask != 0) {
            if ((entry->handle_cap.rights_mask & required_rights) != required_rights) return 0;
            if (caller_cap->target_object_id != entry->handle_cap.target_object_id) return 0;
            return 1;
        }
        if (caller_cap->target_object_id != entry->node->object_id) return 0;
        return 1;
    }
    // Capability hook
    if (caller_cap->capability_id != 0) {
        if ((caller_cap->rights_mask & required_rights) != required_rights) {
            return 0;
        }
        if (entry->handle_cap.target_object_id != 0 || entry->handle_cap.rights_mask != 0) {
            if ((entry->handle_cap.rights_mask & required_rights) != required_rights) return 0;
            if (caller_cap->target_object_id != entry->handle_cap.target_object_id) return 0;
            return 1;
        }
        if (caller_cap->target_object_id != entry->node->object_id) return 0;
    }
    return 1;
}

int fsd_open_file(const char* path, int flags, capability_t* caller_cap, int* out_fd);

int fsd_openat_file(int dirfd, const char* path, int flags, capability_t* caller_cap, int* out_fd) {
    // Stub implementation for Phase B.2
    // Full path resolution relative to dirfd will be added later.
    // For now, if dirfd is a valid special value (e.g. AT_FDCWD), fallback to fsd_open_file.
    // Otherwise, return error to establish the contract without implementing full relative path walking.
    if (dirfd == -100) { // Assuming -100 as AT_FDCWD equivalent for now
        return fsd_open_file(path, flags, caller_cap, out_fd);
    }
    return -1; // Unimplemented path resolution
}

int fsd_open_file(const char* path, int flags, capability_t* caller_cap, int* out_fd) {
    vfs_node_t *node;
    uint32_t requested_rights = 0;
    int found_slot = -1;

    if (!path || !caller_cap || !out_fd) return -1;
    node = vfs_resolve_mount_path(path, caller_cap);
    if (!node) return -2;

    if (flags & VFS_OPEN_READ) requested_rights |= 1;
    if (flags & VFS_OPEN_WRITE) requested_rights |= 2;

    if ((caller_cap->rights_mask & requested_rights) != requested_rights && caller_cap->capability_id != 0) return -3;
    if (caller_cap->target_object_id != node->object_id && caller_cap->capability_id != 0) return -3;

    while (__atomic_test_and_set(&g_open_files_lock, __ATOMIC_ACQUIRE)) {}
    for (size_t i = 0; i < VFS_MAX_OPEN_FILES; ++i) {
        if (!g_open_files[i].in_use) {
            g_open_files[i].in_use = 1;
            found_slot = (int)i;
            break;
        }
    }
    __atomic_clear(&g_open_files_lock, __ATOMIC_RELEASE);

    if (found_slot == -1) return -4;

    size_t i = found_slot;
    g_open_files[i].flags = flags;
    g_open_files[i].offset = 0;
    g_open_files[i].node = node;

    if (node->ops && node->ops->open) {
        if (node->ops->open(node, &g_open_files[i], flags) != 0) {
            g_open_files[i].node = NULL;
            __atomic_store_n(&g_open_files[i].in_use, 0, __ATOMIC_RELEASE);
            return -5;
        }
    }

    g_open_files[i].handle_cap = *caller_cap;
    *out_fd = (int)i;
    return 0;
}

int vfs_open(const char* path, int flags) {
    capability_t dummy_cap = {0};
    dummy_cap.rights_mask = 3;
    vfs_node_t *node = vfs_resolve_mount_path(path, &dummy_cap);
    if (node) dummy_cap.target_object_id = node->object_id;
    int fd = -1;
    int err = fsd_open_file(path, flags, &dummy_cap, &fd);
    if (err == 0) return fd;
    return err;
}

int fsd_read_file(int fd, void* buffer, size_t size, capability_t* caller_cap) {
    if (fd < 0 || fd >= VFS_MAX_OPEN_FILES || !caller_cap) return -1;
    vfs_file_t *entry = &g_open_files[fd];
    if (!entry->in_use || !entry->node || !entry->node->ops || !entry->node->ops->read) return -2;
    if ((entry->flags & VFS_OPEN_READ) == 0) return -3;
    if (!vfs_cap_allows_file(entry, caller_cap, 1)) return -4;

    int bytes = entry->node->ops->read(entry, entry->offset, buffer, size);
    if (bytes > 0) __atomic_add_fetch(&entry->offset, (uint64_t)bytes, __ATOMIC_RELAXED);
    return bytes;
}

int vfs_read(int fd, void* buffer, size_t size) {
    capability_t dummy_cap = {0};
    if (fd >= 0 && fd < VFS_MAX_OPEN_FILES) dummy_cap = g_open_files[fd].handle_cap;
    return fsd_read_file(fd, buffer, size, &dummy_cap);
}

int fsd_write_file(int fd, const void* buffer, size_t size, capability_t* caller_cap) {
    if (fd < 0 || fd >= VFS_MAX_OPEN_FILES || !caller_cap) return -1;
    vfs_file_t *entry = &g_open_files[fd];
    if (!entry->in_use || !entry->node || !entry->node->ops || !entry->node->ops->write) return -2;
    if ((entry->flags & VFS_OPEN_WRITE) == 0) return -3;
    if (!vfs_cap_allows_file(entry, caller_cap, 2)) return -4;

    int bytes = entry->node->ops->write(entry, entry->offset, buffer, size);
    if (bytes > 0) __atomic_add_fetch(&entry->offset, (uint64_t)bytes, __ATOMIC_RELAXED);
    return bytes;
}

int vfs_write(int fd, const void* buffer, size_t size) {
    capability_t dummy_cap = {0};
    if (fd >= 0 && fd < VFS_MAX_OPEN_FILES) dummy_cap = g_open_files[fd].handle_cap;
    return fsd_write_file(fd, buffer, size, &dummy_cap);
}

int fsd_close_file(int fd, capability_t* caller_cap) {
    if (fd < 0 || fd >= VFS_MAX_OPEN_FILES || !caller_cap) return -1;
    vfs_file_t *entry = &g_open_files[fd];
    if (!entry->in_use) return -2;
    if (!vfs_cap_allows_file(entry, caller_cap, 0)) return -4;

    if (entry->node && entry->node->ops && entry->node->ops->close) {
        entry->node->ops->close(entry);
    }

    entry->node = NULL;
    entry->handle_cap.capability_id = 0;
    __atomic_store_n(&entry->in_use, 0, __ATOMIC_RELEASE);
    return 0;
}

int vfs_close(int fd) {
    capability_t dummy_cap = {0};
    if (fd >= 0 && fd < VFS_MAX_OPEN_FILES) dummy_cap = g_open_files[fd].handle_cap;
    return fsd_close_file(fd, &dummy_cap);
}

#ifdef TESTING
void vfs_file_test_reset_state(void) {
    for (int i = 0; i < VFS_MAX_OPEN_FILES; i++) {
        g_open_files[i].in_use = 0;
        g_open_files[i].node = NULL;
    }
    g_open_files_lock = 0;
}
#endif
