#include "mm/mem_validator.h"
#include "hal/hal_mmu.h"
#include "console/console_core.h"
#include "kernel/status.h"
#include <string.h>

static mem_model_t validated_model = MEM_MODEL_NONE;
static bool validation_complete = false;

static const char* model_to_str(enum hal_memory_model model) {
    switch (model) {
        case HAL_MEMORY_MODEL_NONE: return "NONE";
        case HAL_MEMORY_MODEL_MPU: return "MPU";
        case HAL_MEMORY_MODEL_MMU_LITE: return "MMU_LITE";
        case HAL_MEMORY_MODEL_MMU_FULL: return "MMU_FULL";
        default: return "UNKNOWN";
    }
}

int mm_validate_model(void) {
    hal_mem_caps_t caps;
    int ret = hal_mem_get_caps(&caps);

    if (ret != 0) {
        console_log(CONSOLE_LEVEL_PANIC, "MEM: Failed to retrieve HAL memory capabilities!\n");
        return K_ERR_FAULT;
    }

    mem_model_t requested = mem_model_get_current();

    console_log(CONSOLE_LEVEL_INFO, "MEM: Initializing memory model validation...\n");
    console_log(CONSOLE_LEVEL_INFO, "MEM: Detected Hardware Model: %s\n", model_to_str(caps.model));
    console_log(CONSOLE_LEVEL_INFO, "MEM: Requested Software Model: %s\n",
                model_to_str((enum hal_memory_model)requested));

    /*
     * Validation Logic:
     * The hardware must support at least the level of the requested software model.
     */
    bool compatible = false;
    switch (requested) {
        case MEM_MODEL_NONE:
            compatible = true;
            break;
        case MEM_MODEL_MPU:
            compatible = (caps.model >= HAL_MEMORY_MODEL_MPU);
            break;
        case MEM_MODEL_MMU_LITE:
            compatible = (caps.model >= HAL_MEMORY_MODEL_MMU_LITE);
            break;
        case MEM_MODEL_MMU_FULL:
            compatible = (caps.model == HAL_MEMORY_MODEL_MMU_FULL);
            break;
        default:
            compatible = false;
            break;
    }

    if (!compatible) {
        console_log(CONSOLE_LEVEL_PANIC, "MEM: CRITICAL - Hardware model %s does not support requested %s!\n",
                    model_to_str(caps.model), model_to_str((enum hal_memory_model)requested));
        return K_ERR_PROFILE_RESTRICTED;
    }

#ifdef CONFIG_IOMMU_REQUIRED
    if (!caps.supports_iommu) {
        console_log(CONSOLE_LEVEL_PANIC, "MEM: CRITICAL - IOMMU required by profile but not detected in HAL!\n");
        return K_ERR_PROFILE_RESTRICTED;
    }
#else
    if (caps.supports_iommu) {
        console_log(CONSOLE_LEVEL_INFO, "MEM: IOMMU detected and available.\n");
    } else {
        console_log(CONSOLE_LEVEL_WARN, "MEM: IOMMU not detected. System will operate in degraded DMA mode.\n");
    }
#endif

    validated_model = requested;
    validation_complete = true;

    console_log(CONSOLE_LEVEL_INFO, "MEM: Memory model validation successful.\n");
    return K_OK;
}

mem_model_t mm_get_validated_model(void) {
    if (!validation_complete) {
        return mem_model_get_current();
    }
    return validated_model;
}
