#ifndef BHARAT_MM_MEM_PROFILE_VALIDATION_H
#define BHARAT_MM_MEM_PROFILE_VALIDATION_H

#include "mm/mem_model.h"
#include "hal/hal_mmu.h"
#include "kernel/status.h"

typedef struct mem_profile_requirements {
    const char *profile_name;
    enum hal_memory_model min_model;
    bool require_user_kernel_isolation;
    bool require_page_permissions;
    bool require_nx;
    bool require_tlb_shootdown_for_smp;
    bool require_dma_mapping;
    bool require_iommu;
} mem_profile_requirements_t;

/**
 * Validates the hardware capabilities against the selected OS profile requirements.
 *
 * @param hal_caps Detected hardware capabilities.
 * @return K_OK on success, error code otherwise.
 */
kstatus_t mem_profile_validate_requirements(const hal_mem_caps_t *hal_caps);

/**
 * Gets the profile name currently active in the build.
 */
const char* mem_profile_get_active_name(void);

#endif /* BHARAT_MM_MEM_PROFILE_VALIDATION_H */
