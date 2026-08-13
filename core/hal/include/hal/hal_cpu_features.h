#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HAL_CPU_FEATURE_MAX_CPUS 256u

typedef enum {
    HAL_CPU_FEATURE_VECTOR = 0,
    HAL_CPU_FEATURE_SCALABLE_VECTOR,
    HAL_CPU_FEATURE_MATRIX,
    HAL_CPU_FEATURE_AES,
    HAL_CPU_FEATURE_SHA,
    HAL_CPU_FEATURE_PMULL,
    HAL_CPU_FEATURE_CRYPTO,
    HAL_CPU_FEATURE_STRONG_ATOMICS,
    HAL_CPU_FEATURE_FAST_TLB_CTX,
    HAL_CPU_FEATURE_BRANCH_PROTECTION,
    HAL_CPU_FEATURE_MEMORY_TAGGING,
    HAL_CPU_FEATURE_CACHE_BLOCK_OPS,
    HAL_CPU_FEATURE_BITMANIP,
    HAL_CPU_FEATURE_FAST_STRING,
    HAL_CPU_FEATURE__COUNT
} hal_cpu_feature_t;

typedef enum {
    HAL_CPU_FEATURE_SCOPE_ANY = 0,
    HAL_CPU_FEATURE_SCOPE_ALL = 1
} hal_cpu_feature_scope_t;

typedef struct {
    uint64_t raw_bits[(HAL_CPU_FEATURE__COUNT + 63u) / 64u];
    uint64_t usable_bits[(HAL_CPU_FEATURE__COUNT + 63u) / 64u];
} hal_cpu_feature_set_t;

typedef size_t (*hal_cpu_current_id_fn_t)(void);

/* Serial-boot publication API. Mutation is rejected after freeze. */
bool hal_cpu_features_begin(hal_cpu_current_id_fn_t current_id);
bool hal_cpu_features_publish(size_t cpu_id, const hal_cpu_feature_set_t *features);
bool hal_cpu_features_freeze(void);
bool hal_cpu_features_is_frozen(void);

bool hal_cpu_has_feature(size_t cpu_id, hal_cpu_feature_t feature);
bool hal_cpu_has_system_feature(hal_cpu_feature_t feature, hal_cpu_feature_scope_t scope);
bool hal_cpu_feature_set_for_cpu(size_t cpu_id, hal_cpu_feature_set_t *out);
bool hal_cpu_feature_set_system(hal_cpu_feature_scope_t scope, hal_cpu_feature_set_t *out);
bool hal_cpu_has_feature_current(hal_cpu_feature_t feature);
bool hal_cpu_has_system_feature_all(hal_cpu_feature_t feature);
bool hal_cpu_has_system_feature_any(hal_cpu_feature_t feature);
