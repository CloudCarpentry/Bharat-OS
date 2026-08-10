#include "hal/hal_hw_caps.h"
#include "hal/hal_cpu_features.h"
#include "hal/hal_tlb.h"
#include "kernel/primitive.h"
#include "kernel/primitive_caps.h"
#include "kernel/status.h"
#include "profile/profile_policy.h"
#include "bharat/kernel/ds/bh_bitmap.h"
#include "lib/base/string.h"

typedef enum {
    KPRIM_STATE_UNINITIALIZED = 0,
    KPRIM_STATE_DISCOVERING,
    KPRIM_STATE_NORMALIZED,
    KPRIM_STATE_FINALIZED
} kprim_registry_state_t;

typedef struct {
    uint32_t version;
    kprim_registry_state_t state;
    hal_hw_caps_t hw_caps;
    hal_tlb_caps_t tlb_caps;
    hal_cpu_feature_set_t cpu_caps_all;
    hal_cpu_feature_set_t cpu_caps_any;

    bh_primitive_support_level_t cap_support[BH_KPRIM_CAP_COUNT];

    // Extracted bitmaps for O(1) query
    uint64_t system_all_bits[(BH_KPRIM_CAP_COUNT + 63) / 64];
    uint64_t system_any_bits[(BH_KPRIM_CAP_COUNT + 63) / 64];
} bh_kprim_registry_t;

static bh_kprim_registry_t g_registry = {0};

static inline void set_bit(uint64_t *bitmap, bh_kprim_capability_t cap) {
    bitmap[cap / 64] |= (1ULL << (cap % 64));
}

static inline bool get_bit(const uint64_t *bitmap, bh_kprim_capability_t cap) {
    return (bitmap[cap / 64] & (1ULL << (cap % 64))) != 0;
}

kstatus_t bh_kernel_primitive_registry_init(const hal_hw_caps_t *caps) {
    if (!caps) {
        return K_ERR_INVALID_ARG;
    }

    if (!hal_hw_caps_is_frozen() || caps != hal_get_internal_hw_caps()) {
        return K_ERR_IN_PROGRESS;
    }

    if (g_registry.state == KPRIM_STATE_FINALIZED) {
        return K_ERR_BAD_STATE; // Already finalized
    }

    g_registry.state = KPRIM_STATE_DISCOVERING;
    g_registry.hw_caps = *caps;

    // Fetch TLB capabilities
    const hal_tlb_caps_t *tlb_caps = hal_tlb_caps();
    if (tlb_caps) {
        g_registry.tlb_caps = *tlb_caps;
    } else {
        memset(&g_registry.tlb_caps, 0, sizeof(hal_tlb_caps_t));
    }

    // Fetch CPU feature sets
    bool cpu_all_ready = hal_cpu_feature_set_system(HAL_CPU_FEATURE_SCOPE_ALL, &g_registry.cpu_caps_all);
    bool cpu_any_ready = hal_cpu_feature_set_system(HAL_CPU_FEATURE_SCOPE_ANY, &g_registry.cpu_caps_any);

    if (!cpu_all_ready || !cpu_any_ready) {
        g_registry.state = KPRIM_STATE_UNINITIALIZED;
        return K_ERR_IN_PROGRESS;
    }

    g_registry.state = KPRIM_STATE_NORMALIZED;

    // Normalize HAL truth into KPRIM capability truth
    memset(g_registry.system_all_bits, 0, sizeof(g_registry.system_all_bits));
    memset(g_registry.system_any_bits, 0, sizeof(g_registry.system_any_bits));
    memset(g_registry.cap_support, 0, sizeof(g_registry.cap_support));

    // Helper macro for capabilities
    #define NORMALIZE_CAP(CAP, IS_ALL, IS_ANY, SUPPORT) \
        do { \
            if (IS_ALL) set_bit(g_registry.system_all_bits, CAP); \
            if (IS_ANY) set_bit(g_registry.system_any_bits, CAP); \
            g_registry.cap_support[CAP] = SUPPORT; \
        } while(0)

    // Helper to check CPU feature in a set
    #define HAS_CPU_FEATURE(SET_PTR, FEAT) \
        (((SET_PTR)->usable_bits[(FEAT) / 64] & (1ULL << ((FEAT) % 64))) != 0)

    // ATOMIC_64
    bool atomic64_all = HAS_CPU_FEATURE(&g_registry.cpu_caps_all, HAL_CPU_FEATURE_STRONG_ATOMICS);
    bool atomic64_any = HAS_CPU_FEATURE(&g_registry.cpu_caps_any, HAL_CPU_FEATURE_STRONG_ATOMICS);
    NORMALIZE_CAP(BH_KPRIM_CAP_ATOMIC_64, atomic64_all, atomic64_any,
                  atomic64_any ? BH_PRIMITIVE_HARDWARE_ASSISTED : BH_PRIMITIVE_UNSUPPORTED);

    // TLB_PAGE_INVALIDATE
    NORMALIZE_CAP(BH_KPRIM_CAP_TLB_PAGE_INVALIDATE, g_registry.tlb_caps.supports_page_flush, g_registry.tlb_caps.supports_page_flush,
                  g_registry.tlb_caps.supports_page_flush ? BH_PRIMITIVE_HARDWARE_ASSISTED : BH_PRIMITIVE_UNSUPPORTED);

    // TLB_RANGE_INVALIDATE
    NORMALIZE_CAP(BH_KPRIM_CAP_TLB_RANGE_INVALIDATE, g_registry.tlb_caps.supports_range_flush, g_registry.tlb_caps.supports_range_flush,
                  g_registry.tlb_caps.supports_range_flush ? BH_PRIMITIVE_HARDWARE_ASSISTED : BH_PRIMITIVE_UNSUPPORTED);

    // TLB_CONTEXT_INVALIDATE
    NORMALIZE_CAP(BH_KPRIM_CAP_TLB_CONTEXT_INVALIDATE, g_registry.tlb_caps.supports_asid_selective_flush, g_registry.tlb_caps.supports_asid_selective_flush,
                  g_registry.tlb_caps.supports_asid_selective_flush ? BH_PRIMITIVE_HARDWARE_ASSISTED : BH_PRIMITIVE_UNSUPPORTED);

    // TLB_BROADCAST_INVALIDATE
    NORMALIZE_CAP(BH_KPRIM_CAP_TLB_BROADCAST_INVALIDATE, g_registry.tlb_caps.supports_broadcast_flush, g_registry.tlb_caps.supports_broadcast_flush,
                  g_registry.tlb_caps.supports_broadcast_flush ? BH_PRIMITIVE_HARDWARE_ASSISTED : BH_PRIMITIVE_UNSUPPORTED);

    // LAZY_TLB_GENERATION
    NORMALIZE_CAP(BH_KPRIM_CAP_LAZY_TLB_GENERATION, g_registry.tlb_caps.supports_lazy_generation_model, g_registry.tlb_caps.supports_lazy_generation_model,
                  g_registry.tlb_caps.supports_lazy_generation_model ? BH_PRIMITIVE_HARDWARE_ASSISTED : BH_PRIMITIVE_UNSUPPORTED);

    // HIGH_RES_TIMER
    NORMALIZE_CAP(BH_KPRIM_CAP_HIGH_RES_TIMER, g_registry.hw_caps.has_high_res_timer, g_registry.hw_caps.has_high_res_timer,
                  g_registry.hw_caps.has_high_res_timer ? BH_PRIMITIVE_HARDWARE_ASSISTED : BH_PRIMITIVE_UNSUPPORTED);

    // IOMMU
    NORMALIZE_CAP(BH_KPRIM_CAP_IOMMU, g_registry.hw_caps.has_iommu, g_registry.hw_caps.has_iommu,
                  g_registry.hw_caps.has_iommu ? BH_PRIMITIVE_HARDWARE_ENFORCED : BH_PRIMITIVE_UNSUPPORTED);

    #undef NORMALIZE_CAP
    #undef HAS_CPU_FEATURE

    g_registry.version = 1;
    g_registry.state = KPRIM_STATE_FINALIZED;

    return K_OK;
}

bh_primitive_support_level_t bh_kernel_primitive_get_support_level(bh_kernel_primitive_class_t primitive) {
    if (g_registry.state != KPRIM_STATE_FINALIZED) {
        return BH_PRIMITIVE_UNSUPPORTED;
    }

    switch (primitive) {
        case BH_PRIMITIVE_SCHED:
            return BH_PRIMITIVE_SOFTWARE_FALLBACK; // Core scheduler is software

        case BH_PRIMITIVE_MEMORY:
            if (g_registry.hw_caps.has_mmu) {
                return BH_PRIMITIVE_HARDWARE_ENFORCED;
            } else if (g_registry.hw_caps.has_mpu) {
                return BH_PRIMITIVE_HARDWARE_ASSISTED;
            }
            return BH_PRIMITIVE_SOFTWARE_FALLBACK;

        case BH_PRIMITIVE_CAPABILITY:
            return BH_PRIMITIVE_SOFTWARE_FALLBACK;

        case BH_PRIMITIVE_IPC:
            return BH_PRIMITIVE_SOFTWARE_FALLBACK;

        case BH_PRIMITIVE_TIMER:
            if (g_registry.hw_caps.has_high_res_timer) {
                return BH_PRIMITIVE_HARDWARE_ASSISTED;
            }
            return BH_PRIMITIVE_SOFTWARE_FALLBACK;

        case BH_PRIMITIVE_FAULT:
            return BH_PRIMITIVE_HARDWARE_ENFORCED; // CPU traps

        case BH_PRIMITIVE_DMA:
            if (g_registry.hw_caps.has_iommu) {
                return BH_PRIMITIVE_HARDWARE_ENFORCED;
            }
            // DMA fallback depends on policy, but level is software fallback if no IOMMU
            return BH_PRIMITIVE_SOFTWARE_FALLBACK;

        case BH_PRIMITIVE_ACCEL:
            if (g_registry.hw_caps.has_accel_device) {
                return BH_PRIMITIVE_HARDWARE_ASSISTED;
            }
            return BH_PRIMITIVE_UNSUPPORTED;

        case BH_PRIMITIVE_TELEMETRY:
            return BH_PRIMITIVE_SOFTWARE_FALLBACK;

        default:
            return BH_PRIMITIVE_UNSUPPORTED;
    }
}

bool bh_kernel_primitive_available(bh_kernel_primitive_class_t primitive) {
    return bh_kernel_primitive_get_support_level(primitive) != BH_PRIMITIVE_UNSUPPORTED;
}

bool bh_kprim_has(bh_kprim_capability_t cap) {
    if (g_registry.state != KPRIM_STATE_FINALIZED) {
        return false;
    }
    if (cap >= BH_KPRIM_CAP_COUNT) {
        return false;
    }
    return get_bit(g_registry.system_all_bits, cap);
}

bool bh_kprim_has_local(bh_kprim_capability_t cap) {
    if (g_registry.state != KPRIM_STATE_FINALIZED) {
        return false;
    }
    if (cap >= BH_KPRIM_CAP_COUNT) {
        return false;
    }

    if (get_bit(g_registry.system_all_bits, cap)) {
        return true;
    }

    if (!get_bit(g_registry.system_any_bits, cap)) {
        return false;
    }

    if (cap == BH_KPRIM_CAP_ATOMIC_64) {
        return hal_cpu_has_feature_current(HAL_CPU_FEATURE_STRONG_ATOMICS);
    }

    return get_bit(g_registry.system_any_bits, cap);
}

bool bh_kprim_has_any(bh_kprim_capability_t cap) {
    if (g_registry.state != KPRIM_STATE_FINALIZED) {
        return false;
    }
    if (cap >= BH_KPRIM_CAP_COUNT) {
        return false;
    }
    return get_bit(g_registry.system_any_bits, cap);
}

bh_primitive_support_level_t bh_kprim_get_support_level(bh_kprim_capability_t cap) {
    if (g_registry.state != KPRIM_STATE_FINALIZED) {
        return BH_PRIMITIVE_UNSUPPORTED;
    }
    if (cap >= BH_KPRIM_CAP_COUNT) {
        return BH_PRIMITIVE_UNSUPPORTED;
    }
    return g_registry.cap_support[cap];
}
