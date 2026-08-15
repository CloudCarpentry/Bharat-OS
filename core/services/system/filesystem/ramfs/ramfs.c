#include "ramfs.h"
#include <stdlib.h>
#include <string.h>
#include "fs/file.h"

// Define a simple ramfs node structure
typedef struct ramfs_node {
    vfs_node_t vnode;
    char* data;
    size_t allocated_size;
    struct ramfs_node* next_sibling;
    struct ramfs_node* first_child;
} ramfs_internal_node_t;

static vfs_operations_t ramfs_ops;

static void ramfs_memcpy(void *dest, const void *src, size_t n) {
    char *d = (char*)dest;
    const char *s = (const char*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
}

static void ramfs_memset(void *s, int c, size_t n) {
    char *p = (char*)s;
    for (size_t i = 0; i < n; i++) {
        p[i] = c;
    }
}

static ramfs_internal_node_t* create_ramfs_node(const char* name, int is_dir) {
    ramfs_internal_node_t* node = (ramfs_internal_node_t*)malloc(sizeof(ramfs_internal_node_t));
    if (!node) return NULL;
    ramfs_memset(node, 0, sizeof(ramfs_internal_node_t));

    // safe string copy
    size_t i = 0;
    while(name[i] != '\0' && i < sizeof(node->vnode.name) - 1) {
        node->vnode.name[i] = name[i];
        i++;
    }
    node->vnode.name[i] = '\0';

    node->vnode.flags = is_dir ? 2 : 1; // 2 for dir, 1 for file (simplified)
    node->vnode.fs_data = node;
    node->vnode.ops = &ramfs_ops;

    return node;
}

static int ramfs_mount(vfs_mount_t* mnt, vfs_node_t* dev_node) {
    (void)dev_node;
    ramfs_internal_node_t* root = create_ramfs_node("/", 1);
    if (!root) return -1;
    mnt->root_node = &root->vnode;
    return 0;
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

static vfs_node_t* ramfs_lookup(vfs_node_t* dir, const char* name) {
    if (!dir || dir->flags != 2) return NULL;
    ramfs_internal_node_t* pdir = (ramfs_internal_node_t*)dir->fs_data;
    ramfs_internal_node_t* child = pdir->first_child;
    while (child) {
        if (vfs_streq(child->vnode.name, name)) {
            return &child->vnode;
        }
        child = child->next_sibling;
    }
    return NULL;
}

static int ramfs_create(vfs_node_t* dir, const char* name, int flags) {
    if (!dir || dir->flags != 2) return -1;
    ramfs_internal_node_t* pdir = (ramfs_internal_node_t*)dir->fs_data;

    // Check if exists
    if (ramfs_lookup(dir, name)) return -1;

    ramfs_internal_node_t* new_node = create_ramfs_node(name, flags & 0x10); // Simplified check for dir
    if (!new_node) return -1;

    new_node->next_sibling = pdir->first_child;
    pdir->first_child = new_node;

    return 0;
}

static int ramfs_open(vfs_node_t* node, vfs_file_t* file, int flags) {
    (void)flags;
    file->node = node;
    file->offset = 0;
    return 0;
}

static int ramfs_close(vfs_file_t* file) {
    file->node = NULL;
    if (file->private_data) {
        free(file->private_data);
        file->private_data = NULL;
    }
    return 0;
}

static int ramfs_read(vfs_file_t* file, uint64_t offset, void* buffer, size_t size) {
    if (!file || !file->node || !buffer) return -1;
    ramfs_internal_node_t* pnode = (ramfs_internal_node_t*)file->node->fs_data;

    if (offset >= pnode->vnode.size) return 0;

    size_t bytes_to_read = size;
    if (offset + size > pnode->vnode.size) {
        bytes_to_read = pnode->vnode.size - offset;
    }

    ramfs_memcpy(buffer, pnode->data + offset, bytes_to_read);
    return bytes_to_read;
}

static int ramfs_write(vfs_file_t* file, uint64_t offset, const void* buffer, size_t size) {
    if (!file || !file->node || !buffer) return -1;
    ramfs_internal_node_t* pnode = (ramfs_internal_node_t*)file->node->fs_data;

    // Check for integer overflow
    if (offset > ~(size_t)0 - size) return -1;

    size_t new_size = offset + size;
    if (new_size > pnode->allocated_size) {
        char* new_data = (char*)realloc(pnode->data, new_size);
        if (!new_data) return -1;
        pnode->data = new_data;
        // Zero out the gap between old size and new offset to prevent heap information leak
        if (offset > pnode->vnode.size) {
            ramfs_memset(pnode->data + pnode->vnode.size, 0, offset - pnode->vnode.size);
        }
        pnode->allocated_size = new_size;
    }

    ramfs_memcpy(pnode->data + offset, buffer, size);
    if (new_size > pnode->vnode.size) {
        pnode->vnode.size = new_size;
    }

    return size;
}

static void free_node_recursive(ramfs_internal_node_t* node) {
    if (!node) return;

    // Recursively free children
    ramfs_internal_node_t* child = node->first_child;
    while (child) {
        ramfs_internal_node_t* next = child->next_sibling;
        free_node_recursive(child);
        child = next;
    }

    if (node->data) free(node->data);
    free(node);
}

static int ramfs_remove(vfs_node_t* dir, const char* name) {
    if (!dir || dir->flags != 2) return -1;
    ramfs_internal_node_t* pdir = (ramfs_internal_node_t*)dir->fs_data;
    ramfs_internal_node_t* child = pdir->first_child;
    ramfs_internal_node_t* prev = NULL;

    while (child) {
        if (vfs_streq(child->vnode.name, name)) {
            // Unlink
            if (prev) {
                prev->next_sibling = child->next_sibling;
            } else {
                pdir->first_child = child->next_sibling;
            }
            // Free recursively to prevent leaks
            free_node_recursive(child);
            return 0;
        }
        prev = child;
        child = child->next_sibling;
    }
    return -1;
}

static struct dirent* ramfs_readdir(vfs_file_t* file, uint32_t index) {
    if (!file || !file->node || file->node->flags != 2) return NULL;
    ramfs_internal_node_t* pdir = (ramfs_internal_node_t*)file->node->fs_data;
    ramfs_internal_node_t* child = pdir->first_child;

    uint32_t i = 0;
    while (child) {
        if (i == index) {
            if (!file->private_data) {
                file->private_data = malloc(sizeof(struct dirent));
                if (!file->private_data) return NULL;
            }
            struct dirent* d = (struct dirent*)file->private_data;
            d->d_ino = child->vnode.inode;
            size_t k = 0;
            while(child->vnode.name[k] != '\0' && k < sizeof(d->d_name) - 1) {
                d->d_name[k] = child->vnode.name[k];
                k++;
            }
            d->d_name[k] = '\0';
            return d;
        }
        child = child->next_sibling;
        i++;
    }
    return NULL;
}

static int ramfs_getattr(vfs_node_t* node, void* stat_buf) {
    if (!node || !stat_buf) return -1;
    // We could fill up a proper stat struct here, for now it is a stub
    // that returns success
    return 0;
}

static vfs_operations_t ramfs_ops = {
    .mount = ramfs_mount,
    .lookup = ramfs_lookup,
    .create = ramfs_create,
    .remove = ramfs_remove,
    .open = ramfs_open,
    .close = ramfs_close,
    .read = ramfs_read,
    .write = ramfs_write,
    .readdir = ramfs_readdir,
    .getattr = ramfs_getattr,
};

int ramfs_init(void) {
    vfs_driver_info_t info = {
        .name = "ramfs",
        .backend_type = VFS_BACKEND_PSEUDO,
    };
    return vfs_register_driver(&info);
}
