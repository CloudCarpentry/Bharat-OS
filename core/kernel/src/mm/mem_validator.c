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

kstatus_t mem_runtime_validate_profile(const mem_runtime_caps_t *caps,
                                        const mem_profile_contract_t *contract) {
    if (!caps || !contract) return K_ERR_INVALID_ARG;

    // 1. Fundamental Model Check
    bool model_compatible = false;
    switch (contract->model) {
        case MEM_MODEL_MMU_FULL:
            model_compatible = caps->supports_mmu;
            break;
        case MEM_MODEL_MMU_LITE:
            model_compatible = caps->supports_mmu_lite;
            break;
        case MEM_MODEL_MPU:
            model_compatible = caps->supports_mpu;
            break;
        case MEM_MODEL_NONE:
            model_compatible = true;
            break;
        default:
            return K_ERR_PROFILE_RESTRICTED;
    }

    if (!model_compatible) return K_ERR_PROFILE_RESTRICTED;

    // 2. Feature Guarantee Check
    if (contract->require_demand_paging && !caps->supports_demand_paging) return K_ERR_PROFILE_RESTRICTED;
    if (contract->require_page_protection && !caps->supports_page_protection) return K_ERR_PROFILE_RESTRICTED;
    if (contract->require_region_protection && !caps->supports_region_protection) return K_ERR_PROFILE_RESTRICTED;
    if (contract->require_shared_memory && !caps->supports_shared_memory) return K_ERR_PROFILE_RESTRICTED;
    if (contract->require_iommu && !caps->supports_iommu) return K_ERR_PROFILE_RESTRICTED;
    if (contract->require_numa && !caps->supports_numa) return K_ERR_PROFILE_RESTRICTED;
    if (contract->require_hugepage && !caps->supports_hugepage) return K_ERR_PROFILE_RESTRICTED;

    return K_OK;
}

const char* mem_runtime_validation_status_to_string(kstatus_t status) {
    switch (status) {
        case K_OK: return "PASS";
        case K_ERR_PROFILE_RESTRICTED: return "FAIL: Profile Mismatch";
        case K_ERR_INVALID_ARG: return "FAIL: Invalid Arguments";
        case K_ERR_UNSUPPORTED: return "FAIL: Unsupported";
        default: return "FAIL: Unknown Error";
    }
}

int mm_validate_model(void) {
    hal_mem_caps_t hal_caps;
    mem_runtime_caps_t runtime_caps;
    mem_profile_contract_t contract;
    kstatus_t status;

    status = hal_mem_get_caps(&hal_caps);
    if (status != K_OK) {
        console_log(CONSOLE_LEVEL_PANIC, "MEM: Failed to retrieve HAL memory capabilities!\n");
        return status;
    }

    status = mem_runtime_caps_from_hal(&hal_caps, &runtime_caps);
    if (status != K_OK) {
        console_log(CONSOLE_LEVEL_PANIC, "MEM: Failed to normalize memory capabilities!\n");
        return status;
    }

    status = mem_profile_contract_from_build(&contract);
    if (status != K_OK) {
        console_log(CONSOLE_LEVEL_PANIC, "MEM: Failed to derive build profile contract!\n");
        return status;
    }

    console_log(CONSOLE_LEVEL_INFO, "MEM: Initializing memory model validation...\n");
    console_log(CONSOLE_LEVEL_INFO, "MEM: Detected Hardware Model: %s\n", model_to_str(hal_caps.model));
    console_log(CONSOLE_LEVEL_INFO, "MEM: Requested Software Model: %s\n",
                model_to_str((enum hal_memory_model)contract.model));

    status = mem_runtime_validate_profile(&runtime_caps, &contract);

    if (status != K_OK) {
        console_log(CONSOLE_LEVEL_PANIC, "MEM: CRITICAL - Runtime profile validation %s!\n",
                    mem_runtime_validation_status_to_string(status));
        return status;
    }

    validated_model = contract.model;
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
