#include "hal/hal_mmu.h"
#include <stddef.h>

bool hal_memory_model_supported(enum hal_memory_model model) {
    hal_mem_caps_t caps;
    if (hal_mem_get_caps(&caps) != 0) {
        return false;
    }

    /*
     * Hierarchical support check:
     * FULL MMU can generally act as LITE or MPU (via subsets of functionality).
     * MPU-only cannot act as MMU.
     */
    switch (model) {
        case HAL_MEMORY_MODEL_NONE:
            return true;
        case HAL_MEMORY_MODEL_MPU:
            return (caps.model >= HAL_MEMORY_MODEL_MPU);
        case HAL_MEMORY_MODEL_MMU_LITE:
            return (caps.model >= HAL_MEMORY_MODEL_MMU_LITE);
        case HAL_MEMORY_MODEL_MMU_FULL:
            return (caps.model == HAL_MEMORY_MODEL_MMU_FULL);
        default:
            return false;
    }
}
