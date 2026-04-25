#ifndef BHARAT_HAL_MMU_H
#define BHARAT_HAL_MMU_H

#include <stdbool.h>
#include <stdint.h>

#define HAL_PAGE_SIZE_4K (1U << 0)
#define HAL_PAGE_SIZE_2M (1U << 1)
#define HAL_PAGE_SIZE_4M (1U << 2)
#define HAL_PAGE_SIZE_1G (1U << 3)
/**
 * HAL Memory Models
 *
 * Defines the hardware's fundamental memory protection capability.
 */
enum hal_memory_model {
    HAL_MEMORY_MODEL_NONE = 0,
    HAL_MEMORY_MODEL_MPU,       /* Region-based (PMP/MPU) */
    HAL_MEMORY_MODEL_MMU_LITE,  /* Restricted/Simplified MMU */
    HAL_MEMORY_MODEL_MMU_FULL,  /* Full sparse page-table MMU */
};

/**
 * HAL Memory Capabilities
 *
 * Detailed hardware features reported by the architecture/platform.
 */
typedef struct hal_mem_caps {
    enum hal_memory_model model;

    /* Addressing and Translation */
    uint32_t va_bits;
    uint32_t pa_bits;
    uint32_t page_sizes_mask;   /* Bitmask of supported page sizes (HAL_PAGE_SIZE_*) */

    /* Features */
    bool supports_nx;           /* No-Execute / Execute-Never support */
    bool supports_asid;         /* Address Space ID / PCID support */
    bool supports_global;       /* Global bit support */
    bool supports_user_mode;    /* Hardware distinguishes User/Supervisor access */
    bool supports_write_protect;/* Hardware supports Read-Only mappings */

    /* Advanced Features */
    bool supports_iommu;        /* System has at least one IOMMU/SMMU */
    bool supports_hugepages;    /* Support for large (>4K) translations */
    bool supports_dirty_accessed;/* Hardware manages Dirty/Accessed bits */

    /* Implementation-specific details */
    uint32_t max_mpu_regions;   /* For MPU-only models */
} hal_mem_caps_t;

/**
 * Retrieves the current hardware memory capabilities.
 *
 * @param caps Pointer to structure to be populated.
 * @return 0 on success, negative error code on failure.
 */
int hal_mem_get_caps(hal_mem_caps_t *caps);

/**
 * Check if a specific memory model is supported by the hardware.
 */
bool hal_memory_model_supported(enum hal_memory_model model);

#endif /* BHARAT_HAL_MMU_H */
