#include "bharat/stacks/storage/profile.h"

#define STORAGE_MB(x) ((uint64_t)(x) * 1024ULL * 1024ULL)
#define STORAGE_GB(x) ((uint64_t)(x) * 1024ULL * 1024ULL * 1024ULL)

static uint32_t clamp_u32(uint32_t value, uint32_t lo, uint32_t hi) {
    if (value < lo) {
        return lo;
    }
    if (value > hi) {
        return hi;
    }
    return value;
}

int storage_profile_resolve(storage_app_profile_t app_profile,
                            uint64_t device_size_bytes,
                            storage_hw_arch_t arch,
                            storage_profile_config_t* out_cfg) {
    if (!out_cfg || device_size_bytes == 0U) {
        return -1;
    }

    out_cfg->app_profile = app_profile;
    out_cfg->hw_arch = arch;
    out_cfg->device_size_bytes = device_size_bytes;
    out_cfg->enable_numa_locality = 0U;
    out_cfg->enable_journaling = 0U;

    switch (app_profile) {
        case STORAGE_APP_PROFILE_RT:
            out_cfg->fs_backend = (device_size_bytes <= STORAGE_GB(8))
                                      ? STORAGE_FS_BACKEND_LITTLEFS
                                      : STORAGE_FS_BACKEND_FAT;
            out_cfg->cache_policy = STORAGE_CACHE_WRITETHROUGH;
            out_cfg->io_queue_depth = 1U;
            out_cfg->cache_block_budget = (device_size_bytes <= STORAGE_MB(128)) ? 32U : 128U;
            break;

        case STORAGE_APP_PROFILE_DATACENTER:
            out_cfg->fs_backend = (device_size_bytes >= STORAGE_GB(1024))
                                      ? STORAGE_FS_BACKEND_BLOB
                                      : STORAGE_FS_BACKEND_XFS_LIKE;
            out_cfg->cache_policy = STORAGE_CACHE_NUMA_WRITEBACK;
            out_cfg->io_queue_depth = (device_size_bytes >= STORAGE_GB(256)) ? 256U : 128U;
            out_cfg->cache_block_budget = (device_size_bytes >= STORAGE_GB(64)) ? 16384U : 4096U;
            out_cfg->enable_numa_locality = 1U;
            out_cfg->enable_journaling = 1U;
            break;

        case STORAGE_APP_PROFILE_EDGE:
        default:
            if (device_size_bytes <= STORAGE_GB(2)) {
                out_cfg->fs_backend = STORAGE_FS_BACKEND_FAT;
            } else {
                out_cfg->fs_backend = STORAGE_FS_BACKEND_EXT4_LIKE;
            }
            out_cfg->cache_policy = STORAGE_CACHE_WRITEBACK;
            out_cfg->io_queue_depth = (device_size_bytes <= STORAGE_GB(16)) ? 16U : 32U;
            out_cfg->cache_block_budget = (device_size_bytes <= STORAGE_GB(1)) ? 128U : 1024U;
            out_cfg->enable_journaling = 1U;
            break;
    }

    if (arch == STORAGE_HW_ARCH_ARM32 || arch == STORAGE_HW_ARCH_RISCV32) {
        out_cfg->io_queue_depth = clamp_u32(out_cfg->io_queue_depth, 1U, 32U);
        out_cfg->cache_block_budget = clamp_u32(out_cfg->cache_block_budget, 32U, 1024U);
    } else if (arch == STORAGE_HW_ARCH_UNKNOWN) {
        out_cfg->io_queue_depth = clamp_u32(out_cfg->io_queue_depth, 1U, 64U);
        out_cfg->cache_block_budget = clamp_u32(out_cfg->cache_block_budget, 32U, 4096U);
    }

    return 0;
}

const char* storage_profile_backend_name(storage_fs_backend_t backend) {
    switch (backend) {
        case STORAGE_FS_BACKEND_RAMFS:
            return "ramfs";
        case STORAGE_FS_BACKEND_LITTLEFS:
            return "littlefs";
        case STORAGE_FS_BACKEND_FAT:
            return "fat";
        case STORAGE_FS_BACKEND_EXT4_LIKE:
            return "ext4-like";
        case STORAGE_FS_BACKEND_XFS_LIKE:
            return "xfs-like";
        case STORAGE_FS_BACKEND_BLOB:
            return "blob";
        default:
            return "unknown";
    }
}

const char* storage_profile_cache_policy_name(storage_cache_policy_t policy) {
    switch (policy) {
        case STORAGE_CACHE_WRITETHROUGH:
            return "write-through";
        case STORAGE_CACHE_WRITEBACK:
            return "write-back";
        case STORAGE_CACHE_NUMA_WRITEBACK:
            return "numa-write-back";
        default:
            return "unknown";
    }
}
