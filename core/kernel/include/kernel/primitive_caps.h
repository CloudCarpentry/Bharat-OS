#ifndef BHARAT_KERNEL_PRIMITIVE_CAPS_H
#define BHARAT_KERNEL_PRIMITIVE_CAPS_H

#include <stdbool.h>
#include <stdint.h>
#include "kernel/primitive.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Kernel Primitive Capabilities.
 * Architecture-neutral capability descriptors for kernel algorithms to determine
 * whether a hardware-assisted implementation exists.
 */
typedef enum {
    /* Atomic */
    BH_KPRIM_CAP_ATOMIC_64,

    /* Memory Translation / TLB */
    BH_KPRIM_CAP_TLB_PAGE_INVALIDATE,
    BH_KPRIM_CAP_TLB_RANGE_INVALIDATE,
    BH_KPRIM_CAP_TLB_CONTEXT_INVALIDATE,
    BH_KPRIM_CAP_TLB_BROADCAST_INVALIDATE,
    BH_KPRIM_CAP_LAZY_TLB_GENERATION,

    /* Timer */
    BH_KPRIM_CAP_HIGH_RES_TIMER,

    /* DMA / Memory */
    BH_KPRIM_CAP_IOMMU,

    BH_KPRIM_CAP_COUNT
} bh_kprim_capability_t;

/**
 * KPRIM capability check for SYSTEM_ALL.
 * Safe for generic migratable kernel code. True only if ALL eligible CPUs support it.
 */
bool bh_kprim_has(bh_kprim_capability_t cap);

/**
 * KPRIM capability check for LOCAL CPU.
 * Only safe for pinned/local dispatch. True if the CURRENT CPU supports it.
 */
bool bh_kprim_has_local(bh_kprim_capability_t cap);

/**
 * KPRIM capability check for SYSTEM_ANY.
 * True if ANY CPU supports it. Used for discovery/diagnostics/special placement only.
 */
bool bh_kprim_has_any(bh_kprim_capability_t cap);

/**
 * Get the fine-grained implementation support level of a KPRIM capability.
 */
bh_primitive_support_level_t bh_kprim_get_support_level(bh_kprim_capability_t cap);

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_KERNEL_PRIMITIVE_CAPS_H */
