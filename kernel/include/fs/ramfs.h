#ifndef BHARAT_FS_RAMFS_H
#define BHARAT_FS_RAMFS_H

#include "fs/vfs.h"
#include <stddef.h>

// Register the ramfs driver with the VFS.
int ramfs_register_driver(void);

// Create a new ramfs instance and return its root node.
vfs_node_t* ramfs_create_instance(void);

#endif // BHARAT_FS_RAMFS_H
