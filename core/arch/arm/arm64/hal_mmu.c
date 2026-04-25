#include "hal/hal_mmu.h"
#include <string.h>

int hal_mem_get_caps(hal_mem_caps_t *caps) {
    if (!caps) return -1;

    memset(caps, 0, sizeof(hal_mem_caps_t));

    caps->model = HAL_MEMORY_MODEL_MMU_FULL;
    caps->va_bits = 48; // VMSAv8-48
    caps->pa_bits = 48;
    caps->page_sizes_mask = HAL_PAGE_SIZE_4K | HAL_PAGE_SIZE_2M | HAL_PAGE_SIZE_1G;

    caps->supports_nx = true;    // XN
    caps->supports_asid = true;
    caps->supports_global = true;
    caps->supports_user_mode = true;
    caps->supports_write_protect = true;
    caps->supports_hugepages = true;
    caps->supports_dirty_accessed = true;
    caps->supports_iommu = true; // SMMU

    return 0;
}
