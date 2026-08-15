#include "hal/hal_cpu_features.h"

/*
 * Serial boot owns mutation.  Once frozen, every field is immutable and may
 * be read lock-free by any core.  There is deliberately no unfreeze path.
 */
static hal_cpu_feature_set_t g_per_cpu[HAL_CPU_FEATURE_MAX_CPUS];
static bool g_present[HAL_CPU_FEATURE_MAX_CPUS];
static hal_cpu_feature_set_t g_all;
static hal_cpu_feature_set_t g_any;
static hal_cpu_current_id_fn_t g_current_id;
static bool g_frozen;

static void bytes_zero(void *ptr, size_t size) {
    uint8_t *bytes = ptr;
    while (size-- != 0u) *bytes++ = 0u;
}

static void set_copy(hal_cpu_feature_set_t *dst, const hal_cpu_feature_set_t *src) {
    for (size_t i = 0; i < sizeof(*dst) / sizeof(uint64_t); ++i)
        ((uint64_t *)dst)[i] = ((const uint64_t *)src)[i];
}

bool hal_cpu_features_begin(hal_cpu_current_id_fn_t current_id) {
    if (g_frozen || current_id == NULL) return false;
    bytes_zero(g_per_cpu, sizeof(g_per_cpu));
    bytes_zero(g_present, sizeof(g_present));
    bytes_zero(&g_all, sizeof(g_all));
    bytes_zero(&g_any, sizeof(g_any));
    g_current_id = current_id;
    return true;
}

bool hal_cpu_features_publish(size_t cpu_id, const hal_cpu_feature_set_t *features) {
    if (g_frozen || features == NULL || cpu_id >= HAL_CPU_FEATURE_MAX_CPUS) return false;
    set_copy(&g_per_cpu[cpu_id], features);
    g_present[cpu_id] = true;
    return true;
}

bool hal_cpu_features_freeze(void) {
    bool first = true;
    if (g_frozen) return false;
    for (size_t cpu = 0; cpu < HAL_CPU_FEATURE_MAX_CPUS; ++cpu) {
        if (!g_present[cpu]) continue;
        if (first) {
            set_copy(&g_all, &g_per_cpu[cpu]);
            set_copy(&g_any, &g_per_cpu[cpu]);
            first = false;
            continue;
        }
        for (size_t i = 0; i < sizeof(g_all) / sizeof(uint64_t); ++i) {
            ((uint64_t *)&g_all)[i] &= ((const uint64_t *)&g_per_cpu[cpu])[i];
            ((uint64_t *)&g_any)[i] |= ((const uint64_t *)&g_per_cpu[cpu])[i];
        }
    }
    if (first) return false;
    __atomic_store_n(&g_frozen, true, __ATOMIC_RELEASE);
    return true;
}

bool hal_cpu_features_is_frozen(void) {
    return __atomic_load_n(&g_frozen, __ATOMIC_ACQUIRE);
}

bool hal_cpu_feature_set_for_cpu(size_t cpu_id, hal_cpu_feature_set_t *out) {
    if (out == NULL || cpu_id >= HAL_CPU_FEATURE_MAX_CPUS || !g_present[cpu_id]) return false;
    set_copy(out, &g_per_cpu[cpu_id]);
    return true;
}

bool hal_cpu_feature_set_system(hal_cpu_feature_scope_t scope, hal_cpu_feature_set_t *out) {
    if (out == NULL || !hal_cpu_features_is_frozen() || scope > HAL_CPU_FEATURE_SCOPE_ALL) return false;
    set_copy(out, scope == HAL_CPU_FEATURE_SCOPE_ANY ? &g_any : &g_all);
    return true;
}

static bool set_has(const hal_cpu_feature_set_t *set, hal_cpu_feature_t feature) {
    return feature < HAL_CPU_FEATURE__COUNT &&
           (set->usable_bits[(size_t)feature / 64u] & (1ULL << ((size_t)feature % 64u))) != 0u;
}

bool hal_cpu_has_feature(size_t cpu_id, hal_cpu_feature_t feature) {
    hal_cpu_feature_set_t set;
    return hal_cpu_feature_set_for_cpu(cpu_id, &set) && set_has(&set, feature);
}

bool hal_cpu_has_system_feature(hal_cpu_feature_t feature, hal_cpu_feature_scope_t scope) {
    hal_cpu_feature_set_t set;
    return hal_cpu_feature_set_system(scope, &set) && set_has(&set, feature);
}

bool hal_cpu_has_feature_current(hal_cpu_feature_t feature) {
    return g_current_id != NULL && hal_cpu_has_feature(g_current_id(), feature);
}

bool hal_cpu_has_system_feature_all(hal_cpu_feature_t feature) {
    return hal_cpu_has_system_feature(feature, HAL_CPU_FEATURE_SCOPE_ALL);
}

bool hal_cpu_has_system_feature_any(hal_cpu_feature_t feature) {
    return hal_cpu_has_system_feature(feature, HAL_CPU_FEATURE_SCOPE_ANY);
}
