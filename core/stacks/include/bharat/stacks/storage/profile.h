#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    STORAGE_APP_PROFILE_RT = 0,
    STORAGE_APP_PROFILE_EDGE = 1,
    STORAGE_APP_PROFILE_DATACENTER = 2
} storage_app_profile_t;

typedef enum {
    STORAGE_HW_ARCH_UNKNOWN = 0,
    STORAGE_HW_ARCH_ARM32,
    STORAGE_HW_ARCH_ARM64,
    STORAGE_HW_ARCH_RISCV32,
    STORAGE_HW_ARCH_RISCV64,
    STORAGE_HW_ARCH_X86_64
} storage_hw_arch_t;

typedef enum {
    STORAGE_FS_BACKEND_RAMFS = 0,
    STORAGE_FS_BACKEND_LITTLEFS,
    STORAGE_FS_BACKEND_FAT,
    STORAGE_FS_BACKEND_EXT4_LIKE,
    STORAGE_FS_BACKEND_XFS_LIKE,
    STORAGE_FS_BACKEND_BLOB
} storage_fs_backend_t;

typedef enum {
    STORAGE_CACHE_WRITETHROUGH = 0,
    STORAGE_CACHE_WRITEBACK,
    STORAGE_CACHE_NUMA_WRITEBACK
} storage_cache_policy_t;

typedef struct {
    storage_app_profile_t app_profile;
    storage_hw_arch_t hw_arch;
    uint64_t device_size_bytes;

    storage_fs_backend_t fs_backend;
    storage_cache_policy_t cache_policy;
    uint32_t io_queue_depth;
    uint32_t cache_block_budget;
    uint8_t enable_numa_locality;
    uint8_t enable_journaling;
} storage_profile_config_t;

int storage_profile_resolve(storage_app_profile_t app_profile,
                            uint64_t device_size_bytes,
                            storage_hw_arch_t arch,
                            storage_profile_config_t* out_cfg);

const char* storage_profile_backend_name(storage_fs_backend_t backend);
const char* storage_profile_cache_policy_name(storage_cache_policy_t policy);

#ifdef __cplusplus
}
#endif
