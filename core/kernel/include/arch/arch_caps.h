#ifndef BHARAT_ARCH_CAPS_H
#define BHARAT_ARCH_CAPS_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Architecture capability contract flags.
 *
 * These are not CPU micro-feature bits. They describe the kernel-visible
 * architectural/platform contract that common code can rely on.
 */
typedef enum arch_cap {
    ARCH_CAP_64BIT_VA = 0,
    ARCH_CAP_SMP,
    ARCH_CAP_MMU_FULL,
    ARCH_CAP_MMU_LITE,
    ARCH_CAP_MPU_ONLY,
    ARCH_CAP_ASID,
    ARCH_CAP_GLOBAL_TLB_INVALIDATE,
    ARCH_CAP_FINE_GRAIN_PROTECT,
    ARCH_CAP_DMA_COHERENT,
    ARCH_CAP_CACHE_MAINTENANCE,
    ARCH_CAP_DEVICE_MEMORY_ATTRS,
    ARCH_CAP_ADV_IRQ_ROUTING,
    ARCH_CAP_USERSPACE_HIGHHALF,
    ARCH_CAP_HW_CRC,
    ARCH_CAP_SIMD_NET_CSUM,

    ARCH_CAP__COUNT
} arch_cap_t;

typedef struct arch_caps {
    uint64_t bits;
} arch_caps_t;

#define ARCH_CAP_BIT(cap) (1ULL << (uint64_t)(cap))

static inline bool arch_caps_test(arch_caps_t caps, arch_cap_t cap) {
    return (caps.bits & ARCH_CAP_BIT(cap)) != 0;
}

static inline void arch_caps_set(arch_caps_t *caps, arch_cap_t cap) {
    caps->bits |= ARCH_CAP_BIT(cap);
}

static inline void arch_caps_clear(arch_caps_t *caps, arch_cap_t cap) {
    caps->bits &= ~ARCH_CAP_BIT(cap);
}

/*
 * Returns the static capability contract for the currently built architecture
 * and selected platform/profile.
 */
arch_caps_t arch_get_caps(void);

/*
 * Convenience helper for common code.
 */
static inline bool arch_has_cap(arch_cap_t cap) {
    return arch_caps_test(arch_get_caps(), cap);
}

#endif /* BHARAT_ARCH_CAPS_H */
