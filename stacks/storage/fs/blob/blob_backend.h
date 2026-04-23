#ifndef BHARAT_BLOB_BACKEND_H
#define BHARAT_BLOB_BACKEND_H

#include "fs/vfs.h"

#include <stddef.h>

// Register a basic immutable S3-style blob driver descriptor in the VFS registry.
int blob_backend_register_s3_driver(void);

// Create a read-only in-memory blob node for initial immutable object read support.
int blob_backend_init_immutable_node(vfs_node_t *node,
                                     const char *name,
                                     const void *data,
                                     size_t size);

#endif // BHARAT_BLOB_BACKEND_H
