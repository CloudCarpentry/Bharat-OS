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
