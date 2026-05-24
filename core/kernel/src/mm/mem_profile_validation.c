#include "mm/mem_profile_validation.h"
#include "bharat_config.h"
#include <string.h>

static const mem_profile_requirements_t profile_requirements[] = {
#if defined(BHARAT_PROFILE_DESKTOP) || defined(BHARAT_PROFILE_LAPTOP)
    {
        .profile_name = "DESKTOP",
        .min_model = HAL_MEMORY_MODEL_MMU_FULL,
        .require_user_kernel_isolation = true,
        .require_page_permissions = true,
        .require_nx = true,
        .require_tlb_shootdown_for_smp = true,
        .require_dma_mapping = true,
        .require_iommu = false, // Desktop can run without IOMMU but prefers it
    },
#elif defined(BHARAT_PROFILE_AUTOMOTIVE_ECU) || defined(BHARAT_PROFILE_AUTOMOTIVE_INFOTAINMENT)
    {
        .profile_name = "AUTOMOTIVE",
        .min_model = HAL_MEMORY_MODEL_MMU_LITE,
        .require_user_kernel_isolation = true,
        .require_page_permissions = true,
        .require_nx = true,
        .require_tlb_shootdown_for_smp = true,
        .require_dma_mapping = true,
        .require_iommu = false,
    },
#elif defined(BHARAT_PROFILE_EDGE) || defined(BHARAT_PROFILE_ROBOT) || defined(BHARAT_PROFILE_DRONE)
    {
        .profile_name = "EDGE",
        .min_model = HAL_MEMORY_MODEL_MPU,
        .require_user_kernel_isolation = false,
        .require_page_permissions = false,
        .require_nx = false,
        .require_tlb_shootdown_for_smp = false,
        .require_dma_mapping = false,
        .require_iommu = false,
    },
#else
    {
        .profile_name = "GENERIC",
        .min_model = HAL_MEMORY_MODEL_NONE,
        .require_user_kernel_isolation = false,
        .require_page_permissions = false,
        .require_nx = false,
        .require_tlb_shootdown_for_smp = false,
        .require_dma_mapping = false,
        .require_iommu = false,
    },
#endif
};

const char* mem_profile_get_active_name(void) {
    return profile_requirements[0].profile_name;
}

kstatus_t mem_profile_validate_requirements(const hal_mem_caps_t *hal_caps) {
    if (!hal_caps) return K_ERR_INVALID_ARG;

    const mem_profile_requirements_t *req = &profile_requirements[0];

    if (hal_caps->model < req->min_model) return K_ERR_PROFILE_RESTRICTED;

    if (req->require_user_kernel_isolation && !hal_caps->supports_user_kernel_isolation)
        return K_ERR_PROFILE_RESTRICTED;

    if (req->require_page_permissions && !hal_caps->supports_write_protect)
        return K_ERR_PROFILE_RESTRICTED;

    if (req->require_nx && !hal_caps->supports_nx)
        return K_ERR_PROFILE_RESTRICTED;

    if (req->require_tlb_shootdown_for_smp && !hal_caps->supports_tlb_shootdown) {
        // Only fail if SMP is actually enabled/required
#if defined(CONFIG_SMP)
        return K_ERR_PROFILE_RESTRICTED;
#endif
    }

    if (req->require_dma_mapping && !hal_caps->supports_dma_mapping)
        return K_ERR_PROFILE_RESTRICTED;

    if (req->require_iommu && !hal_caps->supports_iommu)
        return K_ERR_PROFILE_RESTRICTED;

    return K_OK;
}
