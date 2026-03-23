#include "fs/ramfs.h"
#include "fs/file.h"
#include "slab.h"
#include <stddef.h>
#include <stdint.h>
#include "mm.h"
#include "lib/string.h"

#define RAMFS_DRIVER_NAME "ramfs"

// Use PAGE_SIZE if defined, else fallback to 4096
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#ifndef ENOMEM
#define ENOMEM 12
#endif

#ifndef ENOSPC
#define ENOSPC 28
#endif

#ifndef CONFIG_RAMFS_MAX_NODES
#define CONFIG_RAMFS_MAX_NODES 0x100000
#endif

typedef struct ramfs_node {
    vfs_node_t vfs_node;
    struct ramfs_node *parent;

    // Type specific data
    union {
        // File data (page-backed block array)
        struct {
            void **blocks;          // Array of pointers, each points to PAGE_SIZE block
            size_t nr_blocks;       // Number of allocated pointers in blocks array (NOT number of pages, but capacity of the array)
            size_t blocks_capacity; // Capacity of the blocks array itself
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

// --- Block-based Storage Helpers ---

// Grow the block array capacity geometrically if needed to accommodate `needed_blocks`
static int ramfs_grow_block_table(ramfs_node_t *inode, size_t needed_blocks) {
    if (needed_blocks <= inode->file.blocks_capacity) return 0;

    size_t old_cap = inode->file.blocks_capacity;
    size_t new_cap;

    if (old_cap == 0) {
        new_cap = 8;
    } else {
        if (old_cap > (SIZE_MAX / 2)) {
            return -ENOMEM;
        }
        new_cap = old_cap * 2;
    }

    while (new_cap < needed_blocks) {
        if (new_cap > (SIZE_MAX / 2)) {
            return -ENOMEM;
        }
        new_cap *= 2;
    }

    if (new_cap > CONFIG_RAMFS_MAX_NODES) {
        new_cap = CONFIG_RAMFS_MAX_NODES;
        if (new_cap < needed_blocks || new_cap <= old_cap) return -ENOSPC;
    }

    if (new_cap > (SIZE_MAX / sizeof(void *))) {
        return -ENOMEM;
    }

    void **new_blocks = (void **)kmalloc(new_cap * sizeof(void *));
    if (!new_blocks) return -ENOMEM;

    // Zero-initialize the entire new array to ensure unallocated blocks are NULL
    memset(new_blocks, 0, new_cap * sizeof(void *));

    if (inode->file.blocks) {
        memcpy(new_blocks, inode->file.blocks, inode->file.nr_blocks * sizeof(void *));
        kfree(inode->file.blocks);
    }

    inode->file.blocks = new_blocks;
    inode->file.blocks_capacity = new_cap;
    return 0;
}

// Ensure the block at `idx` is allocated. Expands table if necessary.
static void *ramfs_ensure_block_index(ramfs_node_t *inode, size_t idx) {
    if (ramfs_grow_block_table(inode, idx + 1) != 0) return NULL;

    if (idx >= inode->file.nr_blocks) {
        inode->file.nr_blocks = idx + 1;
    }

    if (!inode->file.blocks[idx]) {
        // Allocate a new page-sized block
        void *new_block = kmalloc(PAGE_SIZE);
        if (!new_block) return NULL;
        // Zero-fill the new block so holes read as zero
        memset(new_block, 0, PAGE_SIZE);
        inode->file.blocks[idx] = new_block;
    }

    return inode->file.blocks[idx];
}

// Get the block at `idx`. Returns NULL if out of bounds or unallocated.
static void *ramfs_get_block(ramfs_node_t *inode, size_t idx) {
    if (idx >= inode->file.nr_blocks) return NULL;
    return inode->file.blocks[idx];
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
        size_t old_cap = parent->dir.child_capacity;
        size_t new_cap;

        if (old_cap == 0) {
            new_cap = 4;
        } else {
            if (old_cap > (SIZE_MAX / 2)) {
                return -ENOMEM;
            }
            new_cap = old_cap * 2;
        }

        if (new_cap > CONFIG_RAMFS_MAX_NODES) {
            new_cap = CONFIG_RAMFS_MAX_NODES;
            if (new_cap <= old_cap) return -ENOSPC;
        }

        if (new_cap > (SIZE_MAX / sizeof(ramfs_node_t *))) {
            return -ENOMEM;
        }

        ramfs_node_t **new_children = (ramfs_node_t **)kmalloc(sizeof(ramfs_node_t *) * new_cap);
        if (!new_children) return -ENOMEM;

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

    size_t bytes_read = 0;
    uint8_t *dst = (uint8_t *)buffer;

    while (bytes_read < size) {
        uint64_t current_offset = offset + bytes_read;
        size_t block_idx = (size_t)(current_offset / PAGE_SIZE);
        size_t block_offset = (size_t)(current_offset % PAGE_SIZE);
        size_t to_read = PAGE_SIZE - block_offset;

        if (to_read > size - bytes_read) {
            to_read = size - bytes_read;
        }

        void *block_data = ramfs_get_block(rnode, block_idx);
        if (block_data) {
            memcpy(dst + bytes_read, (uint8_t *)block_data + block_offset, to_read);
        } else {
            // Hole, read as zero
            memset(dst + bytes_read, 0, to_read);
        }

        bytes_read += to_read;
    }

    return (int)bytes_read;
}

static int ramfs_write(vfs_file_t *file, uint64_t offset, const void *buffer, size_t size) {
    ramfs_node_t *rnode;

    if (!file || !file->node || !buffer || !file->node->fs_data) return -1;
    rnode = (ramfs_node_t *)file->node->fs_data;

    if (rnode->vfs_node.flags != 1) return -1; // Must be file

    if (size == 0) return 0;

    size_t bytes_written = 0;
    const uint8_t *src = (const uint8_t *)buffer;

    while (bytes_written < size) {
        uint64_t current_offset = offset + bytes_written;
        size_t block_idx = (size_t)(current_offset / PAGE_SIZE);
        size_t block_offset = (size_t)(current_offset % PAGE_SIZE);
        size_t to_write = PAGE_SIZE - block_offset;

        if (to_write > size - bytes_written) {
            to_write = size - bytes_written;
        }

        // Ensure block is allocated (will zero-fill if newly allocated)
        void *block_data = ramfs_ensure_block_index(rnode, block_idx);
        if (!block_data) {
            // Memory allocation failed
            if (bytes_written > 0) break; // Return what was written
            return -1;
        }

        memcpy((uint8_t *)block_data + block_offset, src + bytes_written, to_write);

        bytes_written += to_write;
    }

    if (offset + bytes_written > rnode->vfs_node.size) {
        rnode->vfs_node.size = offset + bytes_written;
    }

    return (int)bytes_written;
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
                    if (child->file.blocks) {
                        for (size_t k = 0; k < child->file.nr_blocks; k++) {
                            if (child->file.blocks[k]) {
                                kfree(child->file.blocks[k]);
                            }
                        }
                        kfree(child->file.blocks);
                    }
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
            size_t new_nr_blocks = (new_size + PAGE_SIZE - 1) / PAGE_SIZE;

            // Free truncated blocks
            for (size_t i = new_nr_blocks; i < rnode->file.nr_blocks; i++) {
                if (rnode->file.blocks[i]) {
                    kfree(rnode->file.blocks[i]);
                    rnode->file.blocks[i] = NULL;
                }
            }
            rnode->file.nr_blocks = new_nr_blocks;

            // Optional: Zero-fill the remainder of the last block
            if (new_size > 0 && rnode->file.blocks) {
                size_t last_block_idx = (new_size - 1) / PAGE_SIZE;
                size_t remainder = new_size % PAGE_SIZE;
                if (remainder != 0 && rnode->file.blocks[last_block_idx]) {
                    memset((uint8_t *)rnode->file.blocks[last_block_idx] + remainder, 0, PAGE_SIZE - remainder);
                }
            }

            rnode->vfs_node.size = new_size;
        } else if (new_size > rnode->vfs_node.size) {
            // zero-fill gap if extending (or let write handle it if done via write)
            // But usually truncate doesn't extend, or if it does, zero-fills.
            // Let's implement extend via our write logic:
            if (new_size > 0) {
                uint64_t offset = new_size - 1;
                uint8_t zero = 0;
                // Writing 1 byte of 0 at end
                return ramfs_write(file, offset, &zero, 1);
            }
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
