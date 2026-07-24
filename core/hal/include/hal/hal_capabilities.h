#ifndef BHARAT_HAL_CAPABILITIES_H
#define BHARAT_HAL_CAPABILITIES_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Bharat-OS Architecture Support Levels (Tiers)
 */
typedef enum bh_arch_support_level {
    BH_ARCH_SUPPORT_SCAFFOLD_ONLY = 0,      /* Tier 0: Folder structure only */
    BH_ARCH_SUPPORT_BUILD_SUPPORTED,        /* Tier 1: Compiles, but not runnable */
    BH_ARCH_SUPPORT_BOOT_SUPPORTED,         /* Tier 2: Reaches kernel_main/early boot */
    BH_ARCH_SUPPORT_RUNTIME_SUPPORTED,       /* Tier 3: Functional scheduling/drivers */
    BH_ARCH_SUPPORT_PRODUCTION_SUPPORTED     /* Tier 4: Validated, secure, production-ready */
} bh_arch_support_level_t;

/**
 * Bharat-OS Memory Models
 */
typedef enum bh_memory_model {
    BH_MEMORY_MODEL_NONE = 0,
    BH_MEMORY_MODEL_MPU_ONLY,       /* Region-based protection (PMP/MPU) */
    BH_MEMORY_MODEL_MMU_LITE,       /* Static/Eager page tables */
    BH_MEMORY_MODEL_MMU_FULL        /* Dynamic/Sparse page tables with VM */
} bh_memory_model_t;

/**
 * HAL Architecture Capabilities Descriptor
 */
typedef struct hal_arch_capabilities {
    const char *arch_name;
    uint32_t arch_bits;             /* 32 or 64 */
    bh_arch_support_level_t support_level;
    bh_memory_model_t memory_model;

    /* Feature Flags */
    bool has_smp;
    bool has_irq_controller;
    bool has_monotonic_timer;
    bool has_cycle_counter;
    bool has_dma;
    bool has_iommu;
    bool has_cache_ops;
    bool has_tlb_ops;

    /* Limits */
    uint32_t max_supported_cores;
} hal_arch_capabilities_t;

/**
 * Retrieves the architecture-specific capability descriptor.
 */
const hal_arch_capabilities_t *hal_get_arch_capabilities(void);

#endif /* BHARAT_HAL_CAPABILITIES_H */
