#include "hal/hal_mmu.h"
#include <string.h>

int hal_mem_get_caps(hal_mem_caps_t *caps) {
    if (!caps) return -1;

    memset(caps, 0, sizeof(hal_mem_caps_t));

    caps->model = HAL_MEMORY_MODEL_MMU_LITE;
    caps->va_bits = 32;
    caps->pa_bits = 32;
    caps->page_sizes_mask = HAL_PAGE_SIZE_4K;

    caps->supports_nx = true;
    caps->supports_asid = false;
    caps->supports_global = true;
    caps->supports_user_mode = true;
    caps->supports_write_protect = true;
    caps->supports_hugepages = false;
    caps->supports_dirty_accessed = false;
    caps->supports_iommu = false;

    return 0;
}
