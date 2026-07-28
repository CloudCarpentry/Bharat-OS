#include <stddef.h>
#include <bharat/cap/cap_authz.h>
#include <bharat/uapi/ipc/status.h>

static uint64_t local_mem_model_get_caps(void) {
#if defined(BHARAT_PROFILE_MPU_ONLY) || defined(CONFIG_MEM_MODEL_MPU) || defined(CONFIG_MEM_MODEL_FLAT)
    return BHARAT_MEM_CAP_REGION_PROTECT;
#elif defined(BHARAT_PROFILE_MMU_LITE) || defined(CONFIG_MEM_MODEL_MMU_LITE)
    return BHARAT_MEM_CAP_VIRT_ADDRSPACE | BHARAT_MEM_CAP_PAGE_MAP | BHARAT_MEM_CAP_PAGE_PROTECT |
           BHARAT_MEM_CAP_TLB_INVALIDATE | BHARAT_MEM_CAP_DMA_MAP;
#else
    return BHARAT_MEM_CAP_VIRT_ADDRSPACE | BHARAT_MEM_CAP_PAGE_MAP | BHARAT_MEM_CAP_PAGE_PROTECT |
           BHARAT_MEM_CAP_DEMAND_FAULT | BHARAT_MEM_CAP_SHARED_ASPACE | BHARAT_MEM_CAP_TLB_INVALIDATE |
           BHARAT_MEM_CAP_DMA_MAP | BHARAT_MEM_CAP_IOMMU | BHARAT_MEM_CAP_PER_CORE_PMM_CACHE;
#endif
}

static bool local_mem_model_has_cap(uint64_t cap) {
    return (local_mem_model_get_caps() & cap) != 0;
}

int32_t bharat_service_dispatch_authorize(
    uint32_t service_id,
    uint32_t opcode,
    const bharat_service_authz_desc_t *descs,
    uint32_t desc_count,
    bharat_cap_handle_t caller_cap,
    uint64_t target_object_id)
{
    (void)service_id;

    if (caller_cap == BHARAT_CAP_INVALID_HANDLE) {
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    const bharat_service_authz_desc_t *desc = NULL;
    for (uint32_t i = 0; i < desc_count; i++) {
        if (descs[i].opcode == opcode) {
            desc = &descs[i];
            break;
        }
    }

    // Deny-by-default for unknown opcodes
    if (!desc) {
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    // 1. Feature / Profile validation where applicable (Memory feature gating)
    if (desc->required_feature_cap != 0) {
        if (!local_mem_model_has_cap(desc->required_feature_cap)) {
            return BHARAT_IPC_STATUS_ERR_UNSUPPORTED; // Fail-closed with explicit error!
        }
    }

    // 2. Validate capability authority
    bharat_cap_scope_t scope = {
        .kind = BHARAT_CAP_SCOPE_OBJECT,
        .scope_id = target_object_id
    };

    bharat_cap_validation_result_t vr = {0};
    bharat_cap_status_t st = bharat_cap_validate(
        caller_cap,
        desc->object_type,
        target_object_id,
        desc->required_rights,
        &scope,
        &vr);

    if (st != BHARAT_CAP_OK || !vr.allowed) {
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    return BHARAT_IPC_STATUS_OK;
}
