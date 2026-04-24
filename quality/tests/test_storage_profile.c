#include <assert.h>
#include <stdint.h>

#include "bharat/stacks/storage/profile.h"

int main(void) {
    storage_profile_config_t cfg;

    assert(storage_profile_resolve(STORAGE_APP_PROFILE_RT,
                                   512ULL * 1024ULL * 1024ULL,
                                   STORAGE_HW_ARCH_ARM32,
                                   &cfg) == 0);
    assert(cfg.fs_backend == STORAGE_FS_BACKEND_LITTLEFS);
    assert(cfg.cache_policy == STORAGE_CACHE_WRITETHROUGH);
    assert(cfg.io_queue_depth == 1U);

    assert(storage_profile_resolve(STORAGE_APP_PROFILE_EDGE,
                                   64ULL * 1024ULL * 1024ULL * 1024ULL,
                                   STORAGE_HW_ARCH_ARM64,
                                   &cfg) == 0);
    assert(cfg.fs_backend == STORAGE_FS_BACKEND_EXT4_LIKE);
    assert(cfg.cache_policy == STORAGE_CACHE_WRITEBACK);
    assert(cfg.enable_journaling == 1U);

    assert(storage_profile_resolve(STORAGE_APP_PROFILE_DATACENTER,
                                   2ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL,
                                   STORAGE_HW_ARCH_X86_64,
                                   &cfg) == 0);
    assert(cfg.fs_backend == STORAGE_FS_BACKEND_BLOB);
    assert(cfg.cache_policy == STORAGE_CACHE_NUMA_WRITEBACK);
    assert(cfg.enable_numa_locality == 1U);

    return 0;
}
