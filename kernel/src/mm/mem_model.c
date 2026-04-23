#include "mm/mem_model.h"

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
