#include "hal/hal_hw_caps.h"
#include "hal/hal_internal.h"
#include "hal/hal_cpu_features.h"
#include "hal/hal_discovery.h"

static hal_hw_caps_t g_internal_hw_caps;
/* BSP boot owns mutation.  Once frozen, all CPUs only read this snapshot. */
static hal_caps_state_t g_caps_state = HAL_CAPS_UNINITIALIZED;

static bool feature_usable(const hal_cpu_feature_set_t *set, hal_cpu_feature_t feature) {
    return feature < HAL_CPU_FEATURE__COUNT &&
           (set->usable_bits[(size_t)feature / 64U] &
            (1ULL << ((size_t)feature % 64U))) != 0U;
}

const hal_hw_caps_t *hal_get_internal_hw_caps(void) {
    return g_caps_state == HAL_CAPS_UNINITIALIZED ? NULL : &g_internal_hw_caps;
}

hal_caps_state_t hal_hw_caps_state(void) {
    return g_caps_state;
}

bool hal_hw_caps_is_frozen(void) {
    return g_caps_state == HAL_CAPS_FROZEN;
}

kstatus_t hal_hw_caps_publish_raw(const hal_hw_caps_t *caps) {
    if (caps == NULL) {
        return K_ERR_INVALID_ARG;
    }
    if (g_caps_state != HAL_CAPS_UNINITIALIZED) {
        return K_ERR_BAD_STATE;
    }
    g_internal_hw_caps = *caps;
    g_caps_state = HAL_CAPS_RAW_DISCOVERED;
    return K_OK;
}

kstatus_t hal_set_internal_hw_caps(const hal_hw_caps_t *caps) {
    return hal_hw_caps_publish_raw(caps);
}

kstatus_t hal_hw_caps_publish_cpu(void) {
    hal_cpu_feature_set_t all;
    hal_cpu_feature_set_t any;
    if (g_caps_state != HAL_CAPS_RAW_DISCOVERED) {
        return K_ERR_BAD_STATE;
    }
    if (!hal_cpu_feature_set_system(HAL_CPU_FEATURE_SCOPE_ALL, &all) ||
        !hal_cpu_feature_set_system(HAL_CPU_FEATURE_SCOPE_ANY, &any)) {
        return K_ERR_IN_PROGRESS;
    }

    g_internal_hw_caps.has_atomic_64 =
        feature_usable(&all, HAL_CPU_FEATURE_STRONG_ATOMICS);
    g_internal_hw_caps.has_vector =
        feature_usable(&all, HAL_CPU_FEATURE_VECTOR);
    g_internal_hw_caps.has_crypto_accel =
        feature_usable(&all, HAL_CPU_FEATURE_CRYPTO) ||
        feature_usable(&all, HAL_CPU_FEATURE_AES) ||
        feature_usable(&all, HAL_CPU_FEATURE_SHA);
    g_caps_state = HAL_CAPS_CPU_FINALIZED;
    return K_OK;
}

kstatus_t hal_hw_caps_finalize(void) {
    if (g_caps_state != HAL_CAPS_CPU_FINALIZED) {
        return K_ERR_BAD_STATE;
    }

    const system_discovery_t *discovery = hal_get_system_discovery();
    g_internal_hw_caps.has_iommu = discovery != NULL && discovery->iommu_count != 0U;
    if (discovery != NULL && discovery->topology.cpu_count != 0U) {
        g_internal_hw_caps.max_cpus = discovery->topology.cpu_count;
    }
    g_caps_state = HAL_CAPS_FROZEN;
    return K_OK;
}
