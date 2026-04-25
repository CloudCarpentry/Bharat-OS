#include "hal/hal_mmu.h"
#include <string.h>

int hal_mem_get_caps(hal_mem_caps_t *caps) {
    if (!caps) return -1;

    memset(caps, 0, sizeof(hal_mem_caps_t));

    caps->model = HAL_MEMORY_MODEL_MMU_FULL;
    caps->va_bits = 39; // Sv39 (most common for riscv64)
    caps->pa_bits = 56;
    caps->page_sizes_mask = HAL_PAGE_SIZE_4K | HAL_PAGE_SIZE_2M | HAL_PAGE_SIZE_1G;

    caps->supports_nx = true;
    caps->supports_asid = true;
    caps->supports_global = true;
    caps->supports_user_mode = true;
    caps->supports_write_protect = true;
    caps->supports_hugepages = true;
    caps->supports_dirty_accessed = true;
    caps->supports_iommu = true; // IOMMU

    return 0;
}
