#ifndef BHARAT_MM_DMA_GRANT_H
#define BHARAT_MM_DMA_GRANT_H

#include <stdint.h>
#include <stddef.h>
#include "kernel/status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t bh_dma_grant_id_t;

/**
 * DMA Access Rights.
 */
typedef enum {
    BH_DMA_RIGHT_READ  = 1u << 0,
    BH_DMA_RIGHT_WRITE = 1u << 1,
    BH_DMA_RIGHT_EXEC  = 1u << 2,
} bh_dma_rights_t;

/**
 * DMA Grant States.
 */
typedef enum {
    BH_DMA_GRANT_CREATED,
    BH_DMA_GRANT_MAPPED,
    BH_DMA_GRANT_ACTIVE,
    BH_DMA_GRANT_REVOKING,
    BH_DMA_GRANT_REVOKED,
    BH_DMA_GRANT_FAILED,
} bh_dma_grant_state_t;

/**
 * DMA Grant Creation Arguments.
 */
typedef struct {
    uint64_t owner_id;        // process/service/thread/domain owner
    uint64_t device_id;       // device object/binding id
    uintptr_t paddr;
    uintptr_t iova;
    size_t length;
    uint32_t rights;
    uint32_t flags;
} bh_dma_grant_create_args_t;

/**
 * Create a DMA grant.
 * Validates owner, device, and range.
 */
kstatus_t bh_dma_grant_create(
    const bh_dma_grant_create_args_t *args,
    bh_dma_grant_id_t *out_grant
);

/**
 * Map a DMA grant.
 * Creates IOMMU mapping or explicit fallback mapping.
 */
kstatus_t bh_dma_grant_map(
    bh_dma_grant_id_t grant
);

/**
 * Activate a DMA grant.
 * After activation, the device may perform DMA.
 */
kstatus_t bh_dma_grant_activate(
    bh_dma_grant_id_t grant
);

/**
 * Revoke a DMA grant.
 * Stops future access, unmaps, and flushes/invalidates as needed.
 */
kstatus_t bh_dma_grant_revoke(
    bh_dma_grant_id_t grant,
    uint32_t flags
);

/**
 * Destroy a DMA grant.
 * Allowed only after the grant is revoked or inactive.
 */
kstatus_t bh_dma_grant_destroy(
    bh_dma_grant_id_t grant
);

/**
 * Get the current state of a DMA grant.
 */
kstatus_t bh_dma_grant_get_state(
    bh_dma_grant_id_t grant,
    bh_dma_grant_state_t *out_state
);

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_MM_DMA_GRANT_H */
