#include "fs/ramfs.h"
#include "fs/file.h"
#include "slab.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define RAMFS_DRIVER_NAME "ramfs"

typedef struct ramfs_node {
    vfs_node_t vfs_node;
    struct ramfs_node *parent;

    // Type specific data
    union {
        // File data
        struct {
            uint8_t *data;
            size_t capacity;
        } file;

        // Directory data
        struct {
            struct ramfs_node **children;
            size_t child_count;
            size_t child_capacity;
        } dir;
    };
} ramfs_node_t;

static vfs_operations_t ramfs_ops;

static void ramfs_strcpy(char *dst, const char *src, size_t dst_size) {
    size_t i = 0;
    if (!dst || dst_size == 0) return;
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

static int ramfs_streq(const char *a, const char *b) {
    size_t i = 0;
    if (!a || !b) return 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) return 0;
        i++;
    }
    return a[i] == b[i];
}

static ramfs_node_t *ramfs_create_node(const char *name, uint32_t type, ramfs_node_t *parent) {
    ramfs_node_t *node = (ramfs_node_t *)kmalloc(sizeof(ramfs_node_t));
    if (!node) return NULL;

    memset(node, 0, sizeof(ramfs_node_t));
    ramfs_strcpy(node->vfs_node.name, name, sizeof(node->vfs_node.name));
    node->vfs_node.flags = type;
    node->vfs_node.backend_type = VFS_BACKEND_PSEUDO;
    node->vfs_node.ops = &ramfs_ops;
    node->vfs_node.fs_data = node;
    node->parent = parent;

    // Assign a dummy object ID just to satisfy capability rules during tests.
    // Real implementation might allocate unique IDs.
    static uint32_t next_obj_id = 1000;
    node->vfs_node.object_id = next_obj_id++;

    if (type == 2) { // VFS_TYPE_DIR (assume 2 for dir, 1 for file if we don't have standard ones yet)
        node->dir.child_capacity = 4;
        node->dir.children = (ramfs_node_t **)kmalloc(sizeof(ramfs_node_t *) * node->dir.child_capacity);
        if (!node->dir.children) {
            kfree(node);
            return NULL;
        }
    }

    return node;
}

// Find a child directory entry by name
static vfs_node_t* ramfs_finddir(vfs_node_t *node, const char *name) {
    ramfs_node_t *rnode;
    size_t i;

    if (!node || !name || !node->fs_data) return NULL;
    rnode = (ramfs_node_t *)node->fs_data;

    // Check if it's a directory (flags == 2)
    if (rnode->vfs_node.flags != 2) return NULL;

    for (i = 0; i < rnode->dir.child_count; i++) {
        if (ramfs_streq(rnode->dir.children[i]->vfs_node.name, name)) {
            return &rnode->dir.children[i]->vfs_node;
        }
    }
    return NULL;
}

// Add a new child node to a directory. Returns 0 on success.
static int ramfs_add_child(ramfs_node_t *parent, ramfs_node_t *child) {
    if (!parent || parent->vfs_node.flags != 2 || !child) return -1;

    if (parent->dir.child_count >= parent->dir.child_capacity) {
        size_t new_cap = parent->dir.child_capacity * 2;
        if (new_cap == 0) new_cap = 4;
        ramfs_node_t **new_children = (ramfs_node_t **)kmalloc(sizeof(ramfs_node_t *) * new_cap);
        if (!new_children) return -1;

        memcpy(new_children, parent->dir.children, sizeof(ramfs_node_t *) * parent->dir.child_count);
        kfree(parent->dir.children);
        parent->dir.children = new_children;
        parent->dir.child_capacity = new_cap;
    }

    parent->dir.children[parent->dir.child_count++] = child;
    return 0;
}

static int ramfs_read(vfs_file_t *file, uint64_t offset, void *buffer, size_t size) {
    ramfs_node_t *rnode;

    if (!file || !file->node || !buffer || !file->node->fs_data) return -1;
    rnode = (ramfs_node_t *)file->node->fs_data;

    if (rnode->vfs_node.flags != 1) return -1; // Must be file

    if (offset >= rnode->vfs_node.size) return 0;

    if (size > rnode->vfs_node.size - (size_t)offset) {
        size = rnode->vfs_node.size - (size_t)offset;
    }

    memcpy(buffer, rnode->file.data + offset, size);
    return (int)size;
}

static int ramfs_write(vfs_file_t *file, uint64_t offset, const void *buffer, size_t size) {
    ramfs_node_t *rnode;

    if (!file || !file->node || !buffer || !file->node->fs_data) return -1;
    rnode = (ramfs_node_t *)file->node->fs_data;

    if (rnode->vfs_node.flags != 1) return -1; // Must be file

    if (offset + size > rnode->file.capacity) {
        size_t new_cap = rnode->file.capacity;
        if (new_cap == 0) new_cap = 64;
        while (new_cap < offset + size) {
            new_cap *= 2;
        }

        uint8_t *new_data = (uint8_t *)kmalloc(new_cap);
        if (!new_data) return -1;

        if (rnode->file.data) {
            memcpy(new_data, rnode->file.data, (size_t)rnode->vfs_node.size);
            kfree(rnode->file.data);
        }

        // Zero-fill gap
        if (offset > rnode->vfs_node.size) {
            memset(new_data + rnode->vfs_node.size, 0, (size_t)offset - (size_t)rnode->vfs_node.size);
        }

        rnode->file.data = new_data;
        rnode->file.capacity = new_cap;
    } else {
        // Just fill the gap if expanding capacity wasn't needed
        if (offset > rnode->vfs_node.size) {
            memset(rnode->file.data + rnode->vfs_node.size, 0, (size_t)offset - (size_t)rnode->vfs_node.size);
        }
    }

    memcpy(rnode->file.data + offset, buffer, size);

    if (offset + size > rnode->vfs_node.size) {
        rnode->vfs_node.size = offset + size;
    }

    return (int)size;
}

// IOCTL for custom ops like create/unlink/truncate
#define RAMFS_IOCTL_CREATE   1
#define RAMFS_IOCTL_UNLINK   2
#define RAMFS_IOCTL_TRUNCATE 3

typedef struct {
    const char *name;
    uint32_t type; // 1 for file, 2 for dir
} ramfs_create_args_t;

static struct dirent *ramfs_readdir(vfs_file_t *file, uint32_t index) {
    ramfs_node_t *rnode;
    static struct dirent dir;

    if (!file || !file->node || !file->node->fs_data) return NULL;
    rnode = (ramfs_node_t *)file->node->fs_data;

    if (rnode->vfs_node.flags != 2) return NULL; // Must be dir

    if (index >= rnode->dir.child_count) return NULL; // Out of bounds

    ramfs_node_t *child = rnode->dir.children[index];
    dir.d_ino = child->vfs_node.object_id;
    ramfs_strcpy(dir.d_name, child->vfs_node.name, sizeof(dir.d_name));
    return &dir;
}

static int ramfs_ioctl(vfs_file_t *file, int request, void *arg) {
    ramfs_node_t *rnode;

    if (!file || !file->node || !file->node->fs_data) return -1;
    rnode = (ramfs_node_t *)file->node->fs_data;

    if (request == RAMFS_IOCTL_CREATE) {
        if (rnode->vfs_node.flags != 2) return -1; // Must be dir
        ramfs_create_args_t *args = (ramfs_create_args_t *)arg;
        if (!args || !args->name) return -1;

        if (ramfs_finddir(&rnode->vfs_node, args->name) != NULL) {
            return -1; // Already exists
        }

        ramfs_node_t *new_node = ramfs_create_node(args->name, args->type, rnode);
        if (!new_node) return -1;

        if (ramfs_add_child(rnode, new_node) != 0) {
            if (new_node->vfs_node.flags == 2) {
                kfree(new_node->dir.children);
            }
            kfree(new_node);
            return -1;
        }
        return 0;

    } else if (request == RAMFS_IOCTL_UNLINK) {
        if (rnode->vfs_node.flags != 2) return -1; // Must be dir
        const char *name = (const char *)arg;
        if (!name) return -1;

        for (size_t i = 0; i < rnode->dir.child_count; i++) {
            if (ramfs_streq(rnode->dir.children[i]->vfs_node.name, name)) {
                ramfs_node_t *child = rnode->dir.children[i];

                // If it's a directory, require it to be empty
                if (child->vfs_node.flags == 2 && child->dir.child_count > 0) {
                    return -1; // Directory not empty
                }

                // Remove from parent
                for (size_t j = i; j < rnode->dir.child_count - 1; j++) {
                    rnode->dir.children[j] = rnode->dir.children[j+1];
                }
                rnode->dir.child_count--;

                // Free the node
                if (child->vfs_node.flags == 1) { // file
                    if (child->file.data) kfree(child->file.data);
                } else if (child->vfs_node.flags == 2) { // dir
                    if (child->dir.children) kfree(child->dir.children);
                }
                kfree(child);
                return 0;
            }
        }
        return -1; // Not found

    } else if (request == RAMFS_IOCTL_TRUNCATE) {
        if (rnode->vfs_node.flags != 1) return -1; // Must be file
        uint64_t *new_size_ptr = (uint64_t *)arg;
        if (!new_size_ptr) return -1;
        uint64_t new_size = *new_size_ptr;

        if (new_size < rnode->vfs_node.size) {
            rnode->vfs_node.size = new_size;
            // Optionally could shrink capacity, but requirement allows keeping it.
        } else if (new_size > rnode->vfs_node.size) {
            // zero-fill gap if extending (or let write handle it if done via write)
            // But usually truncate doesn't extend, or if it does, zero-fills.
            // Let's implement extend via our write logic:
            uint64_t offset = new_size - 1;
            uint8_t zero = 0;
            // Writing 1 byte of 0 at end
            return ramfs_write(file, offset, &zero, 1);
        }
        return 0;
    }

    return -1;
}

static vfs_operations_t ramfs_ops = {
    .read = ramfs_read,
    .write = ramfs_write,
    .open = NULL,
    .close = NULL,
    .readdir = ramfs_readdir,
    .finddir = ramfs_finddir,
    .ioctl = ramfs_ioctl,
};

int ramfs_register_driver(void) {
    vfs_driver_info_t info;

    info.name = RAMFS_DRIVER_NAME;
    info.backend_type = VFS_BACKEND_PSEUDO;
    info.features.supports_journaling = 0;
    info.features.supports_snapshots = 0;
    info.features.supports_xattrs = 0;
    info.features.supports_acls = 0;
    info.features.supports_posix_hardlinks = 0;
    info.features.supports_sparse_files = 1;
    info.features.supports_mmap = 0;

    return vfs_register_driver(&info);
}

vfs_node_t* ramfs_create_instance(void) {
    // 2 represents directory
    ramfs_node_t *root = ramfs_create_node("/", 2, NULL);
    if (!root) return NULL;
    return &root->vfs_node;
}
