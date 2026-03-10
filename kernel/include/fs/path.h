#ifndef BHARAT_FS_PATH_H
#define BHARAT_FS_PATH_H

#include "fs/vfs.h"
#include "advanced/formal_verif.h"

/*
 * Path resolution structures and rights definition.
 * Used for capability checks across paths.
 */

// Basic path-rooted rights (similar to Unix modes, but distinct from ambient ACLs)
#define PATH_RIGHT_READ      CAP_RIGHT_READ
#define PATH_RIGHT_WRITE     CAP_RIGHT_WRITE
#define PATH_RIGHT_EXECUTE   CAP_RIGHT_EXECUTE
#define PATH_RIGHT_CREATE    (1 << 3)
#define PATH_RIGHT_DELETE    (1 << 4)

// Storage class right definitions (what backends can be used)
typedef enum {
    STORAGE_CLASS_FILESYSTEM = 1,
    STORAGE_CLASS_BLOCK      = 2,
    STORAGE_CLASS_BLOB       = 4,
    STORAGE_CLASS_TMPFS      = 8
} storage_class_rights_t;

// Contains the allowed prefix path and rights for a capability token
typedef struct path_rights {
    char allowed_prefix[256];
    uint32_t rights;
    storage_class_rights_t allowed_classes;
} path_rights_t;

// Validates whether the given capability has the required rights for a path
int vfs_validate_path_rights(const char* path, uint32_t required_rights, capability_t* cap);

#endif // BHARAT_FS_PATH_H