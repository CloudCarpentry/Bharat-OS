#ifndef BHARATLIBC_BSYS_RUNTIME_CAPS_H
#define BHARATLIBC_BSYS_RUNTIME_CAPS_H

#include <standard/stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t abi_version;
    uint32_t structure_size;

    uint64_t feature_bits[4];

    uint32_t architecture;
    uint32_t pointer_width;
    uint32_t memory_model;
    uint32_t execution_profile;

    uint32_t page_size;
    uint32_t cache_line_size;
    uint32_t max_threads;
    uint32_t max_handles;
} bh_bsys_runtime_caps_t;

/* Memory model values */
#define BH_CAP_MEM_MPU       1
#define BH_CAP_MEM_MMU_LITE  2
#define BH_CAP_MEM_MMU_FULL  3

/* Execution profile values */
#define BH_CAP_EXEC_RT       1
#define BH_CAP_EXEC_GP       2
#define BH_CAP_EXEC_MIX      3

/* Feature bits definition */
#define BH_FEATURE_ALLOC_UNBOUNDED  (1ULL << 0)
#define BH_FEATURE_POSIX_IO         (1ULL << 1)
#define BH_FEATURE_PTHREAD          (1ULL << 2)

int bh_bsys_validate_capabilities(const bh_bsys_runtime_caps_t *caps);

#ifdef __cplusplus
}
#endif

#endif /* BHARATLIBC_BSYS_RUNTIME_CAPS_H */
