#ifndef BHARAT_MM_MEM_MODEL_H
#define BHARAT_MM_MEM_MODEL_H

#include <stdint.h>
#include <stdbool.h>
#include "bharat/mem_class.h"
#include "kernel/status.h"

// Forward declaration for HAL capabilities
struct hal_mem_caps;

/**
 * Memory Model Architectures
 *
 * Defines the canonical memory capability models for Bharat-OS.
 */
typedef enum {
    MEM_MODEL_NONE = 0,
    MEM_MODEL_MPU,
    MEM_MODEL_MMU_LITE,
    MEM_MODEL_MMU_FULL
} mem_model_t;

/**
 * Memory Protection Architecture Capabilities
 */
// Note: We avoid enum here for MPA_CAPs to prevent name collision with hal_mpa.h
// which uses #defines for different but similarly named constants.
typedef uint64_t mpa_caps_t;
#define MEM_CAP_NONE                 0
#define MEM_CAP_VIRT_ADDRSPACE       (1ULL << 0)
#define MEM_CAP_PAGE_MAP             (1ULL << 1)
#define MEM_CAP_PAGE_PROTECT         (1ULL << 2)
#define MEM_CAP_REGION_PROTECT       (1ULL << 3)
#define MEM_CAP_DEMAND_FAULT         (1ULL << 4)
#define MEM_CAP_SHARED_ASPACE        (1ULL << 5)
#define MEM_CAP_TLB_INVALIDATE       (1ULL << 6)
#define MEM_CAP_DMA_MAP              (1ULL << 7)
#define MEM_CAP_IOMMU                (1ULL << 8)
#define MEM_CAP_PER_CORE_PMM_CACHE   (1ULL << 9)

/**
 * Memory Runtime Capabilities (Kernel-normalized)
 */
typedef struct mem_runtime_caps {
    bool supports_mmu;
    bool supports_mmu_lite;
    bool supports_mpu;

    bool supports_demand_paging;
    bool supports_page_protection;
    bool supports_region_protection;
    bool supports_shared_memory;

    bool supports_dma_map;
    bool supports_iommu;
    bool supports_numa;
    bool supports_hugepage;
} mem_runtime_caps_t;

/**
 * Memory Profile Contract
 */
typedef struct mem_profile_contract {
    mem_model_t model;

    bool require_demand_paging;
    bool require_page_protection;
    bool require_region_protection;
    bool require_shared_memory;
    bool require_dma_isolation;
    bool require_iommu;
    bool require_numa;
    bool require_hugepage;
} mem_profile_contract_t;

/**
 * API Contract for Unsupported Operations:
 *
 * - Any operation requesting a feature not supported by the current memory model
 *   MUST explicitly fail (e.g., return -ENOTSUP or similar explicit error).
 * - It MUST NOT return a silent success (fake success).
 * - It MUST NOT attempt to emulate or pretend support (e.g., MPU trying to emulate
 *   page-granular faulting).
 */

/**
 * Get the current canonical memory model.
 */
mem_model_t mem_model_get_current(void);

/**
 * Normalizes HAL capabilities into kernel runtime capabilities.
 */
kstatus_t mem_runtime_caps_from_hal(const struct hal_mem_caps *hal_caps, mem_runtime_caps_t *out_caps);

/**
 * Derives the memory profile contract from the current build configuration.
 */
kstatus_t mem_profile_contract_from_build(mem_profile_contract_t *out_contract);

/**
 * Query the capability bits of the current memory model.
 */
uint64_t mem_model_get_caps(void);

/**
 * Check whether a specific capability feature is supported.
 */
static inline bool mem_model_has_cap(mpa_caps_t cap) {
    return (mem_model_get_caps() & cap) != 0;
}

/**
 * Validates whether a memory class is supported by the given memory model.
 */
bool mem_class_is_supported(alloc_class_t cls, mem_model_t model);

#endif // BHARAT_MM_MEM_MODEL_H
