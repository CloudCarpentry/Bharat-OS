#include "mm/mem_model.h"

// For now we map the current memory model based on build configuration.
mem_model_t mem_model_get_current(void) {
#if defined(CONFIG_MEM_MODEL_MPU) || defined(CONFIG_MEM_MODEL_FLAT)
    return MEM_MODEL_MPU;
#elif defined(CONFIG_MEM_MODEL_MMU_LITE)
    return MEM_MODEL_MMU_LITE;
#else
    return MEM_MODEL_MMU_FULL;
#endif
}

uint64_t mem_model_get_caps(void) {
    mem_model_t model = mem_model_get_current();
    switch (model) {
        case MEM_MODEL_MMU_FULL:
            return MPA_CAP_VIRT_ADDRSPACE | MPA_CAP_PAGE_MAP | MPA_CAP_PAGE_PROTECT |
                   MPA_CAP_DEMAND_FAULT | MPA_CAP_SHARED_ASPACE | MPA_CAP_TLB_INVALIDATE |
                   MPA_CAP_DMA_MAP | MPA_CAP_IOMMU | MPA_CAP_PER_CORE_PMM_CACHE;
        case MEM_MODEL_MMU_LITE:
            return MPA_CAP_VIRT_ADDRSPACE | MPA_CAP_PAGE_MAP | MPA_CAP_PAGE_PROTECT |
                   MPA_CAP_TLB_INVALIDATE | MPA_CAP_DMA_MAP;
        case MEM_MODEL_MPU:
            return MPA_CAP_REGION_PROTECT;
        default:
            return MPA_CAP_NONE;
    }
}
