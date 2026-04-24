#ifndef BHARAT_FS_CAPS_H
#define BHARAT_FS_CAPS_H

#include <stdint.h>
#include "../../staging/formal/formal_verif.h" // For capability_t

/*
 * Bharat-OS Filesystem Capability Rights & Tiers
 * Governs capability boundaries for filesystem objects.
 */

// Basic path-rooted capability rights (similar to Unix modes, but distinct from ambient ACLs)
#define FS_CAP_RIGHT_LOOKUP    (1 << 0)
#define FS_CAP_RIGHT_READ      (1 << 1)
#define FS_CAP_RIGHT_WRITE     (1 << 2)
#define FS_CAP_RIGHT_APPEND    (1 << 3)
#define FS_CAP_RIGHT_CREATE    (1 << 4)
#define FS_CAP_RIGHT_DELETE    (1 << 5)
#define FS_CAP_RIGHT_EXECUTE   (1 << 6)
#define FS_CAP_RIGHT_MOUNT     (1 << 7)
#define FS_CAP_RIGHT_REMOUNT   (1 << 8)
#define FS_CAP_RIGHT_ADMIN     (1 << 9)
#define FS_CAP_RIGHT_WATCH     (1 << 10)
#define FS_CAP_RIGHT_SETATTR   (1 << 11)

// Storage class right definitions (what backends can be used)
typedef enum {
    FS_STORAGE_CLASS_FILESYSTEM = 1,
    FS_STORAGE_CLASS_BLOCK      = 2,
    FS_STORAGE_CLASS_BLOB       = 4,
    FS_STORAGE_CLASS_TMPFS      = 8
} fs_storage_class_rights_t;

// Storage Profile Options
typedef enum {
    FS_PROFILE_TINY_RO = 0,
    FS_PROFILE_TINY_RW_LOG,
    FS_PROFILE_APPLIANCE,
    FS_PROFILE_MOBILE,
    FS_PROFILE_AUTOMOTIVE,
    FS_PROFILE_CLOUD
} fs_profile_t;

// Capabilities mapping structure for a given storage profile
typedef struct fs_profile_features {
    fs_profile_t profile;
    uint32_t enable_page_cache;
    uint32_t writeback_policy;     // 0: none/through, 1: async, 2: sync/journaled
    uint32_t mount_mutability;     // 0: static, 1: dynamic
    uint32_t requires_journaling;
    uint32_t enable_dir_watch;
    uint32_t enable_mmap;
    uint32_t max_files_limit;
    uint32_t namespace_complexity; // 0: single, 1: system-only, 2: per-process/chroot
} fs_profile_features_t;

// Contains the allowed prefix path and rights for a capability token
typedef struct fs_path_rights {
    char allowed_prefix[256];
    uint32_t rights;
    fs_storage_class_rights_t allowed_classes;
} fs_path_rights_t;

// Validates whether the given capability has the required rights for a path
int fs_validate_caps(const char* path, uint32_t required_rights, capability_t* cap);

#endif // BHARAT_FS_CAPS_H
