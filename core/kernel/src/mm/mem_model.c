#include "mm/mem_model.h"
#include "hal/hal_mmu.h"
#include <string.h>

// For now we map the current memory model based on build configuration.
mem_model_t mem_model_get_current(void) {
#if defined(BHARAT_PROFILE_MPU_ONLY) || defined(CONFIG_MEM_MODEL_MPU) || defined(CONFIG_MEM_MODEL_FLAT)
    return MEM_MODEL_MPU;
#elif defined(BHARAT_PROFILE_MMU_LITE) || defined(CONFIG_MEM_MODEL_MMU_LITE)
    return MEM_MODEL_MMU_LITE;
#elif defined(BHARAT_PROFILE_MMU_FULL)
    return MEM_MODEL_MMU_FULL;
#else
    return MEM_MODEL_MMU_FULL;
#endif
}

kstatus_t mem_runtime_caps_from_hal(const struct hal_mem_caps *hal_caps, mem_runtime_caps_t *out_caps) {
    if (!hal_caps || !out_caps) return K_ERR_INVALID_ARG;

    memset(out_caps, 0, sizeof(mem_runtime_caps_t));

    out_caps->supports_mmu = (hal_caps->model == HAL_MEMORY_MODEL_MMU_FULL);
    out_caps->supports_mmu_lite = (hal_caps->model == HAL_MEMORY_MODEL_MMU_LITE ||
                                   hal_caps->model == HAL_MEMORY_MODEL_MMU_FULL);
    out_caps->supports_mpu = (hal_caps->model == HAL_MEMORY_MODEL_MPU ||
                               hal_caps->model == HAL_MEMORY_MODEL_MMU_LITE ||
                               hal_caps->model == HAL_MEMORY_MODEL_MMU_FULL);

    out_caps->supports_demand_paging = (hal_caps->model == HAL_MEMORY_MODEL_MMU_FULL);
    out_caps->supports_page_protection = hal_caps->supports_write_protect;
    out_caps->supports_region_protection = (hal_caps->model >= HAL_MEMORY_MODEL_MPU);
    out_caps->supports_shared_memory = (hal_caps->model >= HAL_MEMORY_MODEL_MMU_LITE);

    out_caps->supports_dma_map = true; // Baseline assumption for M0
    out_caps->supports_iommu = hal_caps->supports_iommu;
    out_caps->supports_numa = false; // Not yet reported by HAL
    out_caps->supports_hugepage = hal_caps->supports_hugepages;

    return K_OK;
}

kstatus_t mem_profile_contract_from_build(mem_profile_contract_t *out_contract) {
    if (!out_contract) return K_ERR_INVALID_ARG;

    memset(out_contract, 0, sizeof(mem_profile_contract_t));
    out_contract->model = mem_model_get_current();

    switch (out_contract->model) {
        case MEM_MODEL_MMU_FULL:
            out_contract->require_page_protection = true;
            out_contract->require_region_protection = true;
            out_contract->require_shared_memory = true;
#ifdef CONFIG_DEMAND_PAGING_REQUIRED
            out_contract->require_demand_paging = true;
#endif
#ifdef CONFIG_IOMMU_REQUIRED
            out_contract->require_iommu = true;
#endif
            break;

        case MEM_MODEL_MMU_LITE:
            out_contract->require_region_protection = true;
            out_contract->require_page_protection = true;
            out_contract->require_shared_memory = true;
            out_contract->require_demand_paging = false;
            break;

        case MEM_MODEL_MPU:
            out_contract->require_region_protection = true;
            out_contract->require_page_protection = false;
            out_contract->require_demand_paging = false;
            break;

        default:
            break;
    }

    return K_OK;
}

uint64_t mem_model_get_caps(void) {
    mem_model_t model = mem_model_get_current();
    switch (model) {
        case MEM_MODEL_MMU_FULL:
            return MEM_CAP_VIRT_ADDRSPACE | MEM_CAP_PAGE_MAP | MEM_CAP_PAGE_PROTECT |
                   MEM_CAP_DEMAND_FAULT | MEM_CAP_SHARED_ASPACE | MEM_CAP_TLB_INVALIDATE |
                   MEM_CAP_DMA_MAP | MEM_CAP_IOMMU | MEM_CAP_PER_CORE_PMM_CACHE;
        case MEM_MODEL_MMU_LITE:
            return MEM_CAP_VIRT_ADDRSPACE | MEM_CAP_PAGE_MAP | MEM_CAP_PAGE_PROTECT |
                   MEM_CAP_TLB_INVALIDATE | MEM_CAP_DMA_MAP;
        case MEM_MODEL_MPU:
            return MEM_CAP_REGION_PROTECT;
        default:
            return MEM_CAP_NONE;
    }
}

kstatus_t mem_model_validate_hal_caps(mem_model_t model, const hal_memory_caps_t *hal_caps) {
    if (!hal_caps) return K_ERR_INVALID_ARG;

    switch (model) {
        case MEM_MODEL_MMU_FULL:
            if (!hal_caps->supports_mmu_full) return K_ERR_UNSUPPORTED;
            if (!hal_caps->supports_user_kernel_split) return K_ERR_UNSUPPORTED;
            if (!hal_caps->supports_page_protection) return K_ERR_UNSUPPORTED;
            if (!hal_caps->supports_execute_disable) return K_ERR_UNSUPPORTED;
            break;
        case MEM_MODEL_MMU_LITE:
            if (!hal_caps->supports_mmu_lite && !hal_caps->supports_mmu_full) return K_ERR_UNSUPPORTED;
            break;
        case MEM_MODEL_MPU:
            if (!hal_caps->supports_mpu_only && !hal_caps->supports_mmu_full) return K_ERR_UNSUPPORTED;
            break;
        default:
            break;
    }

    return K_OK;
}
