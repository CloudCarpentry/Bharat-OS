#ifndef BHARAT_HAL_MPA_H
#define BHARAT_HAL_MPA_H

#include <stdint.h>
#include <stddef.h>
#include "../../include/mm.h"

// Memory Protection Architecture (MPA) Capability Bits
#define MPA_CAP_VIRT        (1U << 0)
#define MPA_CAP_ASID        (1U << 1)
#define MPA_CAP_GLOBAL      (1U << 2)
#define MPA_CAP_HUGEPAGE    (1U << 3)
#define MPA_CAP_EXEC_PERM   (1U << 4)
#define MPA_CAP_WRITE       (1U << 5)
#define MPA_CAP_USER        (1U << 6)
#define MPA_CAP_IOMMU       (1U << 7)
#define MPA_CAP_MPU_REGION  (1U << 8)
#define MPA_CAP_DEVICE      (1U << 9)

// -----------------------------------------------------------------------------
// CPU Memory Protection (MMU/MMU-lite/MPU)
// -----------------------------------------------------------------------------

typedef struct mem_protect_cpu_ops {
    // 1. Allocate a page-table node (from the PMM)
    phys_addr_t (*make_table)(uint32_t level);

    // 2. Walk and install one leaf entry
    int (*map_page)(phys_addr_t root, virt_addr_t va, phys_addr_t pa, uint32_t flags);

    // 3. Clear and return the frame
    int (*unmap_page)(phys_addr_t root, virt_addr_t va, phys_addr_t *unmapped_pa);

    // 4. Write the architecture's root register (e.g. CR3, satp, TTBR0)
    void (*set_root)(phys_addr_t root);

    // 5. Perform local TLB invalidation
    void (*flush_tlb_local)(virt_addr_t va, uint16_t asid);
} mem_protect_cpu_ops_t;

// -----------------------------------------------------------------------------
// Device DMA Protection (IOMMU)
// -----------------------------------------------------------------------------

typedef struct mem_protect_iommu_ops {
    // Skeleton hook for IOMMU probing. May return NULL if not supported.
    void* (*probe)(void);
} mem_protect_iommu_ops_t;

// -----------------------------------------------------------------------------
// Unified Memory Protection Architecture (MPA) Abstraction
// -----------------------------------------------------------------------------

typedef struct mem_protect_ops {
    uint32_t supported_caps; // Bitmask of MPA_CAP_*

    mem_protect_cpu_ops_t cpu_ops;
    mem_protect_iommu_ops_t iommu_ops;
} mem_protect_ops_t;

extern mem_protect_ops_t *active_mem_protect;

#endif // BHARAT_HAL_MPA_H
