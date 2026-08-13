#include "hal/hal_cpu_features.h"

static const hal_cpu_feature_provider_t *g_provider;

bool hal_cpu_features_register_provider(const hal_cpu_feature_provider_t *provider) {
    if (provider == NULL || provider->for_cpu == NULL ||
        provider->for_current_cpu == NULL || provider->for_system == NULL) {
        return false;
    }
    const hal_cpu_feature_provider_t *expected = NULL;
    return __atomic_compare_exchange_n(&g_provider, &expected, provider, false,
                                       __ATOMIC_RELEASE, __ATOMIC_RELAXED);
}

bool hal_cpu_feature_set_for_cpu(size_t cpu_id, hal_cpu_feature_set_t *out) {
    if (out == NULL) {
        return false;
    }
    const hal_cpu_feature_provider_t *provider =
        __atomic_load_n(&g_provider, __ATOMIC_ACQUIRE);
    return provider != NULL && provider->for_cpu(cpu_id, out);
}

bool hal_cpu_feature_set_system(hal_cpu_feature_scope_t scope, hal_cpu_feature_set_t *out) {
    if (out == NULL) {
        return false;
    }
    const hal_cpu_feature_provider_t *provider =
        __atomic_load_n(&g_provider, __ATOMIC_ACQUIRE);
    return provider != NULL && scope <= HAL_CPU_FEATURE_SCOPE_ALL &&
           provider->for_system(scope, out);
}

bool hal_cpu_has_feature(size_t cpu_id, hal_cpu_feature_t feature) {
    hal_cpu_feature_set_t set;
    if (!hal_cpu_feature_set_for_cpu(cpu_id, &set) || feature >= HAL_CPU_FEATURE__COUNT) {
        return false;
    }
    return (set.usable_bits[(size_t)feature / 64u] & (1ULL << ((size_t)feature % 64u))) != 0;
}

bool hal_cpu_has_system_feature(hal_cpu_feature_t feature, hal_cpu_feature_scope_t scope) {
    if (scope == HAL_CPU_FEATURE_SCOPE_ANY) {
        return hal_cpu_has_system_feature_any(feature);
    }
    return hal_cpu_has_system_feature_all(feature);
}

bool hal_cpu_has_feature_current(hal_cpu_feature_t feature) {
    hal_cpu_feature_set_t set;
    const hal_cpu_feature_provider_t *provider =
        __atomic_load_n(&g_provider, __ATOMIC_ACQUIRE);
    return provider != NULL && provider->for_current_cpu(&set) &&
           feature < HAL_CPU_FEATURE__COUNT &&
           (set.usable_bits[(size_t)feature / 64u] &
            (1ULL << ((size_t)feature % 64u))) != 0;
}

bool hal_cpu_has_system_feature_all(hal_cpu_feature_t feature) {
    hal_cpu_feature_set_t set;
    if (!hal_cpu_feature_set_system(HAL_CPU_FEATURE_SCOPE_ALL, &set) ||
        feature >= HAL_CPU_FEATURE__COUNT) {
        return false;
    }
    return (set.usable_bits[(size_t)feature / 64u] & (1ULL << ((size_t)feature % 64u))) != 0;
}

bool hal_cpu_has_system_feature_any(hal_cpu_feature_t feature) {
    hal_cpu_feature_set_t set;
    if (!hal_cpu_feature_set_system(HAL_CPU_FEATURE_SCOPE_ANY, &set) ||
        feature >= HAL_CPU_FEATURE__COUNT) {
        return false;
    }
    return (set.usable_bits[(size_t)feature / 64u] & (1ULL << ((size_t)feature % 64u))) != 0;
}
