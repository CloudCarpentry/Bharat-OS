#include "fs/vfs.h"
#include "fs/file.h"
#include "fs/mount.h"
#include "capability.h"
#include "sched/sched.h"

// Migrate logic from kernel/src/fs/file.c

#define VFS_MAX_OPEN_FILES 64

static vfs_node_t* walk_path_relative(vfs_node_t* start_node, const char* path);

#define MAX_PROCESSES 128
typedef struct {
    vfs_file_t files[VFS_MAX_OPEN_FILES];
    uint32_t lock;
} process_fd_table_t;
static process_fd_table_t g_proc_fd_tables[MAX_PROCESSES];
static process_fd_table_t* get_proc_fd_table(capability_t* cap) {
    if (!cap) return &g_proc_fd_tables[0];
    uint32_t pid = cap->capability_id % MAX_PROCESSES;
    return &g_proc_fd_tables[pid];
}

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

void split_path(const char* full_path, char* parent_path, char* leaf_name, size_t max_len);

static vfs_node_t* walk_path_relative(vfs_node_t* start_node, const char* path) {
    if (!start_node || !path) return NULL;

    // Fast path for empty or current dir
    if (path[0] == '\0') return start_node;
    if (path[0] == '.' && path[1] == '\0') return start_node;

    char component[256];
    size_t i = 0, j = 0;
    vfs_node_t* current = start_node;

    while (path[i] == '/') i++; // Skip leading slashes

    while (path[i] != '\0') {
        j = 0;
        while (path[i] != '/' && path[i] != '\0' && j < sizeof(component) - 1) {
            component[j++] = path[i++];
        }
        component[j] = '\0';

        if (j > 0) {
            if (current->ops && current->ops->lookup) {
                current = current->ops->lookup(current, component);
                if (!current) return NULL;
            } else {
                return NULL;
            }
        }

        while (path[i] == '/') i++;
    }

    return current;
}

static int do_open_node(vfs_node_t *node, int flags, capability_t* caller_cap, int* out_fd) {
    uint32_t requested_rights = 0;
    int found_slot = -1;

    if (!node || !caller_cap || !out_fd) return -1;

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

    process_fd_table_t* table = get_proc_fd_table(caller_cap);
    while (__atomic_test_and_set(&table->lock, __ATOMIC_ACQUIRE)) {}
    for (size_t i = 0; i < VFS_MAX_OPEN_FILES; ++i) {
        if (!table->files[i].in_use) {
            table->files[i].in_use = 1;
            found_slot = (int)i;
            break;
        }
    }
    __atomic_clear(&table->lock, __ATOMIC_RELEASE);

    if (found_slot == -1) return -4;

    size_t i = found_slot;
    table->files[i].flags = flags;
    table->files[i].offset = 0;
    table->files[i].node = node;
    table->files[i].private_data = NULL; // initialize to prevent random free

    if (node->ops && node->ops->open) {
        if (node->ops->open(node, &table->files[i], flags) != 0) {
            table->files[i].node = NULL;
            __atomic_store_n(&table->files[i].in_use, 0, __ATOMIC_RELEASE);
            return -5;
        }
    }

    table->files[i].handle_cap = *caller_cap;
    *out_fd = (int)i;
    return 0;
}

int fsd_openat_file(int dirfd, const char* path, int flags, capability_t* caller_cap, int* out_fd) {
    if (!path || !caller_cap || !out_fd) return -1;

    vfs_node_t *dir_node = NULL;

    if (path[0] == '/') {
        dir_node = vfs_resolve_mount_path(path, caller_cap);
        if (!dir_node) return -2;
        // Skip leading slashes for path walking since dir_node is already the mount root
        while (*path == '/') path++;
    } else if (dirfd == -100) { // AT_FDCWD
        // Use vfs_root or resolve root
        dir_node = vfs_resolve_mount_path("/", caller_cap);
        if (!dir_node) return -2;
    } else {
        if (dirfd < 0 || dirfd >= VFS_MAX_OPEN_FILES) return -1;
        process_fd_table_t* table = get_proc_fd_table(caller_cap);
        vfs_file_t *dir_entry = &table->files[dirfd];
        if (!dir_entry->in_use || !dir_entry->node) return -2;
        dir_node = dir_entry->node;
    }

    vfs_node_t *node = walk_path_relative(dir_node, path);

    if (!node) {
        if (flags & VFS_OPEN_CREAT) {
            char parent_path[256];
            char leaf_name[256];
            split_path(path, parent_path, leaf_name, sizeof(parent_path));

            vfs_node_t *parent_node = walk_path_relative(dir_node, parent_path);
            if (!parent_node) return -2;

            if (parent_node->ops && parent_node->ops->create) {
                if (parent_node->ops->create(parent_node, leaf_name, flags) != 0) {
                    return -5;
                }
                node = parent_node->ops->lookup(parent_node, leaf_name);
                if (!node) return -5;
            } else {
                return -5;
            }
        } else {
            return -2; // ENOENT
        }
    }

    return do_open_node(node, flags, caller_cap, out_fd);
}

int fsd_open_file(const char* path, int flags, capability_t* caller_cap, int* out_fd) {
    return fsd_openat_file(-100, path, flags, caller_cap, out_fd);
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
    process_fd_table_t* table = get_proc_fd_table(caller_cap);
    vfs_file_t *entry = &table->files[fd];
    if (!entry->in_use || !entry->node || !entry->node->ops || !entry->node->ops->read) return -2;
    if ((entry->flags & VFS_OPEN_READ) == 0) return -3;
    if (!vfs_cap_allows_file(entry, caller_cap, 1)) return -4;

    int bytes = entry->node->ops->read(entry, entry->offset, buffer, size);
    if (bytes > 0) __atomic_add_fetch(&entry->offset, (uint64_t)bytes, __ATOMIC_RELAXED);
    return bytes;
}

int vfs_read(int fd, void* buffer, size_t size) {
    capability_t dummy_cap = {0};
    if (fd >= 0 && fd < VFS_MAX_OPEN_FILES) dummy_cap = get_proc_fd_table(NULL)->files[fd].handle_cap;
    return fsd_read_file(fd, buffer, size, &dummy_cap);
}

int fsd_write_file(int fd, const void* buffer, size_t size, capability_t* caller_cap) {
    if (fd < 0 || fd >= VFS_MAX_OPEN_FILES || !caller_cap) return -1;
    process_fd_table_t* table = get_proc_fd_table(caller_cap);
    vfs_file_t *entry = &table->files[fd];
    if (!entry->in_use || !entry->node || !entry->node->ops || !entry->node->ops->write) return -2;
    if ((entry->flags & VFS_OPEN_WRITE) == 0) return -3;
    if (!vfs_cap_allows_file(entry, caller_cap, 2)) return -4;

    int bytes = entry->node->ops->write(entry, entry->offset, buffer, size);
    if (bytes > 0) __atomic_add_fetch(&entry->offset, (uint64_t)bytes, __ATOMIC_RELAXED);
    return bytes;
}

int vfs_write(int fd, const void* buffer, size_t size) {
    capability_t dummy_cap = {0};
    if (fd >= 0 && fd < VFS_MAX_OPEN_FILES) dummy_cap = get_proc_fd_table(NULL)->files[fd].handle_cap;
    return fsd_write_file(fd, buffer, size, &dummy_cap);
}

int fsd_close_file(int fd, capability_t* caller_cap) {
    if (fd < 0 || fd >= VFS_MAX_OPEN_FILES || !caller_cap) return -1;
    process_fd_table_t* table = get_proc_fd_table(caller_cap);
    vfs_file_t *entry = &table->files[fd];
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
    if (fd >= 0 && fd < VFS_MAX_OPEN_FILES) dummy_cap = get_proc_fd_table(NULL)->files[fd].handle_cap;
    return fsd_close_file(fd, &dummy_cap);
}

#ifdef TESTING
void vfs_file_test_reset_state(void) {
    for (int i = 0; i < VFS_MAX_OPEN_FILES; i++) {
        get_proc_fd_table(NULL)->files[i].in_use = 0;
        get_proc_fd_table(NULL)->files[i].node = NULL;
        get_proc_fd_table(NULL)->files[i].private_data = NULL;
    }
    get_proc_fd_table(NULL)->lock = 0;
}
#endif

int64_t fsd_lseek_file(int fd, int64_t offset, int whence, capability_t* caller_cap) {
    if (fd < 0 || fd >= VFS_MAX_OPEN_FILES || !caller_cap) return -1;
    process_fd_table_t* table = get_proc_fd_table(caller_cap);
    vfs_file_t *entry = &table->files[fd];
    if (!entry->in_use || !entry->node) return -2;
    // Allow read or write rights for seek
    if (!vfs_cap_allows_file(entry, caller_cap, 1) && !vfs_cap_allows_file(entry, caller_cap, 2)) return -4;

    int64_t new_offset = 0;
    if (whence == 0) { // SEEK_SET
        new_offset = offset;
    } else if (whence == 1) { // SEEK_CUR
        new_offset = (int64_t)__atomic_load_n(&entry->offset, __ATOMIC_RELAXED) + offset;
    } else if (whence == 2) { // SEEK_END
        new_offset = (int64_t)entry->node->size + offset;
    } else {
        return -5;
    }

    if (new_offset < 0) return -6; // EINVAL

    __atomic_store_n(&entry->offset, (uint64_t)new_offset, __ATOMIC_RELAXED);
    return new_offset;
}

int64_t vfs_lseek(int fd, int64_t offset, int whence) {
    capability_t dummy_cap = {0};
    if (fd >= 0 && fd < VFS_MAX_OPEN_FILES) dummy_cap = get_proc_fd_table(NULL)->files[fd].handle_cap;
    return fsd_lseek_file(fd, offset, whence, &dummy_cap);
}

int fsd_fstat_file(int fd, void* stat_buf, capability_t* caller_cap) {
    if (fd < 0 || fd >= VFS_MAX_OPEN_FILES || !caller_cap) return -1;
    process_fd_table_t* table = get_proc_fd_table(caller_cap);
    vfs_file_t *entry = &table->files[fd];
    if (!entry->in_use || !entry->node) return -2;
    if (!vfs_cap_allows_file(entry, caller_cap, 1)) return -4;

    if (entry->node->ops && entry->node->ops->getattr) {
        return entry->node->ops->getattr(entry->node, stat_buf);
    }
    return -5;
}

int vfs_fstat(int fd, void* stat_buf) {
    capability_t dummy_cap = {0};
    if (fd >= 0 && fd < VFS_MAX_OPEN_FILES) dummy_cap = get_proc_fd_table(NULL)->files[fd].handle_cap;
    return fsd_fstat_file(fd, stat_buf, &dummy_cap);
}

void split_path(const char* full_path, char* parent_path, char* leaf_name, size_t max_len);

int vfs_mkdir(const char* path, int mode, capability_t* caller_cap) {
    (void)mode;
    char parent_path[256];
    char leaf_name[256];

    split_path(path, parent_path, leaf_name, sizeof(parent_path));

    vfs_node_t *mount_root = vfs_resolve_mount_path(parent_path, caller_cap);
    if (!mount_root) return -1;

    // Walk relative to mount root
    vfs_node_t *dir = walk_path_relative(mount_root, parent_path);
    if (!dir || !dir->ops || !dir->ops->create) return -1;

    // Basic authorization check - ideally through capabilities hook
    // We assume dir has adequate rights if the mount path resolved.

    return dir->ops->create(dir, leaf_name, 0x10); // 0x10 for dir as used in ramfs
}

int vfs_unlink(const char* path, capability_t* caller_cap) {
    char parent_path[256];
    char leaf_name[256];

    split_path(path, parent_path, leaf_name, sizeof(parent_path));

    vfs_node_t *mount_root = vfs_resolve_mount_path(parent_path, caller_cap);
    if (!mount_root) return -1;

    // Walk relative to mount root
    vfs_node_t *dir = walk_path_relative(mount_root, parent_path);
    if (!dir || !dir->ops || !dir->ops->remove) return -1;

    return dir->ops->remove(dir, leaf_name);
}

struct dirent* vfs_readdir(int fd, uint32_t index, capability_t* caller_cap) {
    if (fd < 0 || fd >= VFS_MAX_OPEN_FILES) return NULL;
    process_fd_table_t* table = get_proc_fd_table(caller_cap);
    vfs_file_t *entry = &table->files[fd];
    if (!entry->in_use || !entry->node) return NULL;
    if (!vfs_cap_allows_file(entry, caller_cap, 1)) return NULL;

    if (entry->node->ops && entry->node->ops->readdir) {
        return entry->node->ops->readdir(entry, index);
    }
    return NULL;
}

int vfs_dup(int oldfd) {
    if (oldfd < 0 || oldfd >= VFS_MAX_OPEN_FILES) return -1;
    process_fd_table_t* table = get_proc_fd_table(NULL);
    vfs_file_t *old_entry = &table->files[oldfd];
    if (!old_entry->in_use) return -2;

    int new_slot = -1;
    while (__atomic_test_and_set(&table->lock, __ATOMIC_ACQUIRE)) {}
    for (size_t i = 0; i < VFS_MAX_OPEN_FILES; ++i) {
        if (!table->files[i].in_use) {
            table->files[i].in_use = 1;
            new_slot = (int)i;
            break;
        }
    }
    __atomic_clear(&table->lock, __ATOMIC_RELEASE);

    if (new_slot == -1) return -4;

    table->files[new_slot] = *old_entry;
    // We shouldn't share private_data between duplicated descriptors since it contains the dirent.
    // Or we should manage reference counting for node and private_data.
    // For now, don't copy private data or set it to NULL.
    table->files[new_slot].private_data = NULL;

    return new_slot;
}

#include "pipe.h"

int vfs_pipe(int pipefd[2]) {
    if (!pipefd) return -1;

    vfs_node_t *rnode, *wnode;
    if (fs_pipe_create(&rnode, &wnode) != 0) return -1;

    int rfd = -1, wfd = -1;
    process_fd_table_t* table = get_proc_fd_table(NULL);
    while (__atomic_test_and_set(&table->lock, __ATOMIC_ACQUIRE)) {}

    for (size_t i = 0; i < VFS_MAX_OPEN_FILES; ++i) {
        if (!table->files[i].in_use) {
            table->files[i].in_use = 1;
            rfd = (int)i;
            break;
        }
    }

    if (rfd != -1) {
        for (size_t i = 0; i < VFS_MAX_OPEN_FILES; ++i) {
            if (!table->files[i].in_use) {
                table->files[i].in_use = 1;
                wfd = (int)i;
                break;
            }
        }
    }

    if (rfd == -1 || wfd == -1) {
        if (rfd != -1) table->files[rfd].in_use = 0;
        if (wfd != -1) table->files[wfd].in_use = 0;
        __atomic_clear(&table->lock, __ATOMIC_RELEASE);
        return -4; // EMFILE
    }

    table->files[rfd].flags = VFS_OPEN_READ;
    table->files[rfd].offset = 0;
    table->files[rfd].node = rnode;
    table->files[rfd].private_data = NULL;

    table->files[wfd].flags = VFS_OPEN_WRITE;
    table->files[wfd].offset = 0;
    table->files[wfd].node = wnode;
    table->files[wfd].private_data = NULL;

    __atomic_clear(&table->lock, __ATOMIC_RELEASE);

    pipefd[0] = rfd;
    pipefd[1] = wfd;

    return 0;
}
