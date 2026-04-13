#ifndef BHARAT_MM_MEM_MODEL_H
#define BHARAT_MM_MEM_MODEL_H

#include <stdint.h>
#include <stdbool.h>

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
typedef enum {
    MPA_CAP_NONE                 = 0,
    MPA_CAP_VIRT_ADDRSPACE       = (1ULL << 0),
    MPA_CAP_PAGE_MAP             = (1ULL << 1),
    MPA_CAP_PAGE_PROTECT         = (1ULL << 2),
    MPA_CAP_REGION_PROTECT       = (1ULL << 3),
    MPA_CAP_DEMAND_FAULT         = (1ULL << 4),
    MPA_CAP_SHARED_ASPACE        = (1ULL << 5),
    MPA_CAP_TLB_INVALIDATE       = (1ULL << 6),
    MPA_CAP_DMA_MAP              = (1ULL << 7),
    MPA_CAP_IOMMU                = (1ULL << 8),
    MPA_CAP_PER_CORE_PMM_CACHE   = (1ULL << 9)
} mpa_caps_t;

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
 * Query the capability bits of the current memory model.
 */
uint64_t mem_model_get_caps(void);

/**
 * Check whether a specific capability feature is supported.
 */
static inline bool mem_model_has_cap(mpa_caps_t cap) {
    return (mem_model_get_caps() & cap) != 0;
}

#endif // BHARAT_MM_MEM_MODEL_H
