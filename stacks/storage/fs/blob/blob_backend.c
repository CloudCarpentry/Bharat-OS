#include "blob_backend.h"
#include "fs/vfs.h"
#include "kernel/status.h"

// PHASE1_COMPAT_SHIM: Blob backend logic moved out of kernel.

int blob_backend_register_s3_driver(void) {
    return K_ERR_REQUIRES_FS_SERVICE;
}

int blob_backend_init_immutable_node(vfs_node_t *node, const char *name, const void *data, size_t size) {
    (void)node;
    (void)name;
    (void)data;
    (void)size;
    return K_ERR_REQUIRES_FS_SERVICE;
}
