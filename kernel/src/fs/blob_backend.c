#include "fs/blob_backend.h"

#include <stddef.h>
#include <stdint.h>

#define BLOB_S3_DRIVER_NAME "s3-compatible"

typedef struct {
    const uint8_t *data;
    size_t size;
} blob_immutable_object_t;

#include "fs/file.h"

static int blob_read(vfs_file_t *file, uint64_t offset, void *buffer, size_t size) {
    blob_immutable_object_t *obj;
    size_t i;

    if (!file || !file->node || !buffer || !file->node->fs_data) {
        return -1;
    }

    obj = (blob_immutable_object_t *)file->node->fs_data;
    if (offset >= obj->size) {
        return 0;
    }

    if (size > obj->size - (size_t)offset) {
        size = obj->size - (size_t)offset;
    }

    for (i = 0; i < size; ++i) {
        ((uint8_t *)buffer)[i] = obj->data[(size_t)offset + i];
    }

    return (int)size;
}

static int blob_write(vfs_file_t *file, uint64_t offset, const void *buffer, size_t size) {
    (void)file;
    (void)offset;
    (void)buffer;
    (void)size;
    return -1;
}

static vfs_operations_t g_blob_ops = {
    .read = blob_read,
    .write = blob_write,
    .open = NULL,
    .close = NULL,
    .readdir = NULL,
    .finddir = NULL,
    .ioctl = NULL,
};

static blob_immutable_object_t g_blob_objects[8];
static size_t g_blob_object_count = 0;

static void copy_name(char *dst, const char *src, size_t dst_size) {
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

int blob_backend_register_s3_driver(void) {
    vfs_driver_info_t info;

    info.name = BLOB_S3_DRIVER_NAME;
    info.backend_type = VFS_BACKEND_BLOB;
    info.features.supports_journaling = 0;
    info.features.supports_snapshots = 0;
    info.features.supports_xattrs = 1;
    info.features.supports_acls = 1;
    info.features.supports_posix_hardlinks = 0;
    info.features.supports_sparse_files = 0;
    info.features.supports_mmap = 0;

    return vfs_register_driver(&info);
}

int blob_backend_init_immutable_node(vfs_node_t *node,
                                     const char *name,
                                     const void *data,
                                     size_t size) {
    blob_immutable_object_t *obj;

    if (!node || !data || size == 0 || g_blob_object_count >= 8) {
        return -1;
    }

    obj = &g_blob_objects[g_blob_object_count++];
    obj->data = (const uint8_t *)data;
    obj->size = size;

    copy_name(node->name, name, sizeof(node->name));
    node->backend_type = VFS_BACKEND_BLOB;
    node->size = size;
    node->ops = &g_blob_ops;
    node->fs_data = obj;
    return 0;
}
