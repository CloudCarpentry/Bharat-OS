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

    /* Enforce deny-by-default on open */
    if (caller_cap->capability_id != 0) {
        if ((caller_cap->rights_mask & requested_rights) != requested_rights) return -3;
        if (caller_cap->target_object_id != node->object_id && caller_cap->target_object_id != VFS_NAMESPACE_OBJECT_ID) return -3;
    }

    /* Enforce mount-level constraints (e.g. read-only mount) */
    vfs_mount_t* mnt = node->mnt_context;
    if (mnt && (mnt->mount_flags & VFS_MOUNT_READONLY) && (flags & VFS_OPEN_WRITE)) {
        return -6; // EROFS
    }

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
    g_open_files[i].private_data = NULL; // initialize to prevent random free

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
        g_open_files[i].private_data = NULL;
    }
    g_open_files_lock = 0;
}
#endif

int fsd_lseek_file(int fd, uint64_t offset, int whence, capability_t* caller_cap) {
    if (fd < 0 || fd >= VFS_MAX_OPEN_FILES || !caller_cap) return -1;
    vfs_file_t *entry = &g_open_files[fd];
    if (!entry->in_use || !entry->node) return -2;
    // Allow read or write rights for seek
    if (!vfs_cap_allows_file(entry, caller_cap, 1) && !vfs_cap_allows_file(entry, caller_cap, 2)) return -4;

    if (whence == 0) { // SEEK_SET
        __atomic_store_n(&entry->offset, offset, __ATOMIC_RELAXED);
    } else if (whence == 1) { // SEEK_CUR
        __atomic_add_fetch(&entry->offset, offset, __ATOMIC_RELAXED);
    } else if (whence == 2) { // SEEK_END
        __atomic_store_n(&entry->offset, entry->node->size + offset, __ATOMIC_RELAXED);
    } else {
        return -5;
    }
    return 0;
}

int vfs_lseek(int fd, uint64_t offset, int whence) {
    capability_t dummy_cap = {0};
    if (fd >= 0 && fd < VFS_MAX_OPEN_FILES) dummy_cap = g_open_files[fd].handle_cap;
    return fsd_lseek_file(fd, offset, whence, &dummy_cap);
}

int fsd_fstat_file(int fd, void* stat_buf, capability_t* caller_cap) {
    if (fd < 0 || fd >= VFS_MAX_OPEN_FILES || !caller_cap) return -1;
    vfs_file_t *entry = &g_open_files[fd];
    if (!entry->in_use || !entry->node) return -2;
    if (!vfs_cap_allows_file(entry, caller_cap, 1)) return -4;

    if (entry->node->ops && entry->node->ops->getattr) {
        return entry->node->ops->getattr(entry->node, stat_buf);
    }
    return -5;
}

int vfs_fstat(int fd, void* stat_buf) {
    capability_t dummy_cap = {0};
    if (fd >= 0 && fd < VFS_MAX_OPEN_FILES) dummy_cap = g_open_files[fd].handle_cap;
    return fsd_fstat_file(fd, stat_buf, &dummy_cap);
}

void split_path(const char* full_path, char* parent_path, char* leaf_name, size_t max_len);

int vfs_mkdir(const char* path, int mode) {
    (void)mode;
    char parent_path[256];
    char leaf_name[256];

    split_path(path, parent_path, leaf_name, sizeof(parent_path));

    capability_t dummy_cap = {0};
    dummy_cap.rights_mask = 3; // Need write right
    vfs_node_t *dir = vfs_resolve_mount_path(parent_path, &dummy_cap);
    if (!dir || !dir->ops || !dir->ops->create) return -1;

    return dir->ops->create(dir, leaf_name, 0x10); // 0x10 for dir as used in ramfs
}

int vfs_unlink(const char* path) {
    char parent_path[256];
    char leaf_name[256];

    split_path(path, parent_path, leaf_name, sizeof(parent_path));

    capability_t dummy_cap = {0};
    dummy_cap.rights_mask = 3;
    vfs_node_t *dir = vfs_resolve_mount_path(parent_path, &dummy_cap);
    if (!dir || !dir->ops || !dir->ops->remove) return -1;
    return dir->ops->remove(dir, leaf_name);
}

struct dirent* vfs_readdir(int fd, uint32_t index) {
    if (fd < 0 || fd >= VFS_MAX_OPEN_FILES) return NULL;
    vfs_file_t *entry = &g_open_files[fd];
    if (!entry->in_use || !entry->node) return NULL;

    if (entry->node->ops && entry->node->ops->readdir) {
        return entry->node->ops->readdir(entry, index);
    }
    return NULL;
}

int vfs_dup(int oldfd) {
    if (oldfd < 0 || oldfd >= VFS_MAX_OPEN_FILES) return -1;
    vfs_file_t *old_entry = &g_open_files[oldfd];
    if (!old_entry->in_use) return -2;

    int new_slot = -1;
    while (__atomic_test_and_set(&g_open_files_lock, __ATOMIC_ACQUIRE)) {}
    for (size_t i = 0; i < VFS_MAX_OPEN_FILES; ++i) {
        if (!g_open_files[i].in_use) {
            g_open_files[i].in_use = 1;
            new_slot = (int)i;
            break;
        }
    }
    __atomic_clear(&g_open_files_lock, __ATOMIC_RELEASE);

    if (new_slot == -1) return -4;

    g_open_files[new_slot] = *old_entry;
    // We shouldn't share private_data between duplicated descriptors since it contains the dirent.
    // Or we should manage reference counting for node and private_data.
    // For now, don't copy private data or set it to NULL.
    g_open_files[new_slot].private_data = NULL;

    return new_slot;
}

#include "pipe.h"

int vfs_pipe(int pipefd[2]) {
    if (!pipefd) return -1;

    vfs_node_t *rnode, *wnode;
    if (fs_pipe_create(&rnode, &wnode) != 0) return -1;

    int rfd = -1, wfd = -1;
    while (__atomic_test_and_set(&g_open_files_lock, __ATOMIC_ACQUIRE)) {}

    for (size_t i = 0; i < VFS_MAX_OPEN_FILES; ++i) {
        if (!g_open_files[i].in_use) {
            g_open_files[i].in_use = 1;
            rfd = (int)i;
            break;
        }
    }

    if (rfd != -1) {
        for (size_t i = 0; i < VFS_MAX_OPEN_FILES; ++i) {
            if (!g_open_files[i].in_use) {
                g_open_files[i].in_use = 1;
                wfd = (int)i;
                break;
            }
        }
    }

    if (rfd == -1 || wfd == -1) {
        if (rfd != -1) g_open_files[rfd].in_use = 0;
        if (wfd != -1) g_open_files[wfd].in_use = 0;
        __atomic_clear(&g_open_files_lock, __ATOMIC_RELEASE);
        return -4; // EMFILE
    }

    g_open_files[rfd].flags = VFS_OPEN_READ;
    g_open_files[rfd].offset = 0;
    g_open_files[rfd].node = rnode;
    g_open_files[rfd].private_data = NULL;

    g_open_files[wfd].flags = VFS_OPEN_WRITE;
    g_open_files[wfd].offset = 0;
    g_open_files[wfd].node = wnode;
    g_open_files[wfd].private_data = NULL;

    __atomic_clear(&g_open_files_lock, __ATOMIC_RELEASE);

    pipefd[0] = rfd;
    pipefd[1] = wfd;

    return 0;
}
