#include "mm/dma_grant.h"
#include "kernel/primitive.h"
#include "hal/hal_hw_caps.h"
#include "mm/iommu.h"
#include <string.h>

#define MAX_DMA_GRANTS 256

typedef struct {
    bh_dma_grant_create_args_t args;
    bh_dma_grant_state_t state;
    bool allocated;
} dma_grant_internal_t;

static dma_grant_internal_t g_dma_grants[MAX_DMA_GRANTS];

kstatus_t bh_dma_grant_create(
    const bh_dma_grant_create_args_t *args,
    bh_dma_grant_id_t *out_grant
) {
    if (!args || !out_grant) return K_ERR_INVALID_ARG;

    // Check if IOMMU is available
    bool iommu_avail = iommu_available();

    // If no IOMMU, only allow if explicitly permitted by profile/policy
    // For now, we strictly require IOMMU or return unsupported if enforcing.
    if (!iommu_avail) {
        // TODO: Consult profile/policy for fallback allowance
        // For this task, we'll allow creation but it might fail at 'map'
    }

    for (uint32_t i = 0; i < MAX_DMA_GRANTS; i++) {
        if (!g_dma_grants[i].allocated) {
            g_dma_grants[i].args = *args;
            g_dma_grants[i].state = BH_DMA_GRANT_CREATED;
            g_dma_grants[i].allocated = true;
            *out_grant = (bh_dma_grant_id_t)i;
            return K_OK;
        }
    }

    return K_ERR_NO_MEMORY;
}

kstatus_t bh_dma_grant_map(
    bh_dma_grant_id_t grant
) {
    if (grant >= MAX_DMA_GRANTS || !g_dma_grants[grant].allocated) {
        return K_ERR_INVALID_ARG;
    }

    dma_grant_internal_t *g = &g_dma_grants[grant];
    if (g->state != BH_DMA_GRANT_CREATED) return K_ERR_BAD_STATE;

    if (iommu_available()) {
        // Look up or create a domain for the owner (Simplified for now)
        bh_iommu_domain_t *domain = NULL;
        kstatus_t ret = iommu_map(domain, g->args.iova, g->args.paddr, g->args.length, g->args.flags);
        if (ret != K_OK) {
            g->state = BH_DMA_GRANT_FAILED;
            return ret;
        }
        g->state = BH_DMA_GRANT_MAPPED;
        return K_OK;
    } else {
        // Fallback logic
        // If paddr == iova, it's a direct mapping (unsafe fallback)
        if (g->args.paddr == g->args.iova) {
             g->state = BH_DMA_GRANT_MAPPED;
             return K_OK;
        }
        return K_ERR_UNSUPPORTED;
    }
}

kstatus_t bh_dma_grant_activate(
    bh_dma_grant_id_t grant
) {
    if (grant >= MAX_DMA_GRANTS || !g_dma_grants[grant].allocated) {
        return K_ERR_INVALID_ARG;
    }

    dma_grant_internal_t *g = &g_dma_grants[grant];
    if (g->state != BH_DMA_GRANT_MAPPED) return K_ERR_BAD_STATE;

    g->state = BH_DMA_GRANT_ACTIVE;
    return K_OK;
}

kstatus_t bh_dma_grant_revoke(
    bh_dma_grant_id_t grant,
    uint32_t flags
) {
    (void)flags;
    if (grant >= MAX_DMA_GRANTS || !g_dma_grants[grant].allocated) {
        return K_ERR_INVALID_ARG;
    }

    dma_grant_internal_t *g = &g_dma_grants[grant];
    g->state = BH_DMA_GRANT_REVOKING;

    if (iommu_available()) {
        bh_iommu_domain_t *domain = NULL;
        (void)iommu_unmap(domain, g->args.iova, g->args.length);
    }

    g->state = BH_DMA_GRANT_REVOKED;
    return K_OK;
}

kstatus_t bh_dma_grant_destroy(
    bh_dma_grant_id_t grant
) {
    if (grant >= MAX_DMA_GRANTS || !g_dma_grants[grant].allocated) {
        return K_ERR_INVALID_ARG;
    }

    dma_grant_internal_t *g = &g_dma_grants[grant];
    if (g->state != BH_DMA_GRANT_REVOKED && g->state != BH_DMA_GRANT_CREATED && g->state != BH_DMA_GRANT_FAILED) {
        return K_ERR_BAD_STATE;
    }

    g->allocated = false;
    return K_OK;
}

kstatus_t bh_dma_grant_get_state(
    bh_dma_grant_id_t grant,
    bh_dma_grant_state_t *out_state
) {
    if (grant >= MAX_DMA_GRANTS || !g_dma_grants[grant].allocated || !out_state) {
        return K_ERR_INVALID_ARG;
    }

    *out_state = g_dma_grants[grant].state;
    return K_OK;
}
