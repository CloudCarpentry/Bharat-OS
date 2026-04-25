#include "hal/hal_mmu.h"
#include <string.h>

int hal_mem_get_caps(hal_mem_caps_t *caps) {
    if (!caps) return -1;

    memset(caps, 0, sizeof(hal_mem_caps_t));

    caps->model = HAL_MEMORY_MODEL_MMU_FULL;
    caps->va_bits = 48; // Standard 4-level paging
    caps->pa_bits = 46; // Physical address bits
    caps->page_sizes_mask = HAL_PAGE_SIZE_4K | HAL_PAGE_SIZE_2M | HAL_PAGE_SIZE_1G;

    caps->supports_nx = true;
    caps->supports_asid = true; // PCID
    caps->supports_global = true;
    caps->supports_user_mode = true;
    caps->supports_write_protect = true;
    caps->supports_hugepages = true;
    caps->supports_dirty_accessed = true;

    // IOMMU depends on chipset (VT-d), but the architecture supports it.
    // For now we report based on architecture capability.
    caps->supports_iommu = true;

    return 0;
}
