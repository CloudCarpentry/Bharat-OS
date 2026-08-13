#include "hal/hal_discovery.h"
#include <hal/hal_cpu_topology.h>
#include "hal/hal_cpu_features.h"
#include "boot/boot_info.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// Global discovery structure populated by early architecture boot code (ACPI or FDT)
static system_discovery_t g_system_discovery;

system_discovery_t* hal_get_system_discovery(void) {
    return &g_system_discovery;
}

bool hal_cpu_topology_query(hal_cpu_topology_info_t *out) {
    if (!out) {
        return false;
    }

    const system_discovery_t *discovery = hal_get_system_discovery();
    uint32_t discovered = 1U;
    if (discovery && discovery->topology.cpu_count > 0U) {
        discovered = discovery->topology.cpu_count;
    }
    if (discovered > BHARAT_MAX_CPUS) {
        discovered = BHARAT_MAX_CPUS;
    }

    /* These masks are a legacy 32-CPU consumer boundary; the authoritative
     * topology count is not truncated to fit them. */
    uint32_t mask_cpus = discovered > 32U ? 32U : discovered;
    uint32_t valid_cpu_mask = (mask_cpus == 32U) ? UINT32_MAX : ((1U << mask_cpus) - 1U);

    out->discovered_cpu_count = discovered;
    out->valid_cpu_mask = valid_cpu_mask;
    out->performance_cluster_mask = out->valid_cpu_mask;
    out->efficiency_cluster_mask = 0;
    out->smp_available = (discovered > 1U);
    out->homogeneous_cores = true;
    return true;
}

void hal_discovery_init(const boot_info_t *boot) {
    if (!boot) return;
    hal_arch_discovery_init(boot);
}

static inline void accel_set(uint64_t *mask, hal_accel_feature_t feat, bool enabled) {
    if (!enabled || feat >= HAL_ACCEL_FEAT__COUNT) {
        return;
    }
    *mask |= (1ULL << (uint32_t)feat);
}

void hal_discovery_publish_cpu_caps(void) {
    system_discovery_t *sys = hal_get_system_discovery();
    hal_cpu_feature_set_t caps_all;
    hal_cpu_feature_set_t caps_any;

    if (!hal_cpu_feature_set_system(HAL_CPU_FEATURE_SCOPE_ALL, &caps_all) ||
        !hal_cpu_feature_set_system(HAL_CPU_FEATURE_SCOPE_ANY, &caps_any)) {
        sys->accel.raw_all_mask = 0;
        sys->accel.raw_any_mask = 0;
        sys->accel.usable_all_mask = 0;
        sys->accel.usable_any_mask = 0;
        return;
    }

    sys->accel.raw_all_mask = 0;
    sys->accel.raw_any_mask = 0;
    sys->accel.usable_all_mask = 0;
    sys->accel.usable_any_mask = 0;

#define EXPORT_ACCEL(accel_feature, cpu_feature)                                  \
    do {                                                                           \
        const uint64_t bit = 1ULL << ((size_t)(cpu_feature) % 64u);                \
        const size_t word = (size_t)(cpu_feature) / 64u;                           \
        accel_set(&sys->accel.raw_all_mask, accel_feature,                         \
                  (caps_all.raw_bits[word] & bit) != 0U);                          \
        accel_set(&sys->accel.raw_any_mask, accel_feature,                         \
                  (caps_any.raw_bits[word] & bit) != 0U);                          \
        accel_set(&sys->accel.usable_all_mask, accel_feature,                      \
                  (caps_all.usable_bits[word] & bit) != 0U);                       \
        accel_set(&sys->accel.usable_any_mask, accel_feature,                      \
                  (caps_any.usable_bits[word] & bit) != 0U);                       \
    } while (0)
    EXPORT_ACCEL(HAL_ACCEL_FEAT_VECTOR, HAL_CPU_FEATURE_VECTOR);
    EXPORT_ACCEL(HAL_ACCEL_FEAT_AES, HAL_CPU_FEATURE_AES);
    EXPORT_ACCEL(HAL_ACCEL_FEAT_SHA, HAL_CPU_FEATURE_SHA);
    EXPORT_ACCEL(HAL_ACCEL_FEAT_PMULL, HAL_CPU_FEATURE_PMULL);
    EXPORT_ACCEL(HAL_ACCEL_FEAT_STRONG_ATOMICS, HAL_CPU_FEATURE_STRONG_ATOMICS);
#undef EXPORT_ACCEL
}
