#ifndef BHARAT_RAMFS_H
#define BHARAT_RAMFS_H

#include "fs/vfs.h"

// Initialize the ramfs driver and register it with VFS
int ramfs_init(void);

#endif // BHARAT_RAMFS_H
