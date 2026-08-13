#include "arch/arch_cpu_caps.h"
#include "arch/common/cpu_caps_state.h"
#include "bharat/cpu_local.h"
#include "hal/hal_cpu_features.h"
#include <stddef.h>
#include <stdint.h>

// Forward declarations from kernel string functions (kernel/src/lib/string.c)
void *memset(void *dest, int c, size_t n);

// Forward declaration (defined arch-specifically in HAL)
extern uint32_t hal_cpu_get_id(void);

static arch_cpu_caps_record_t g_boot_cpu_caps;
static arch_cpu_caps_record_t g_system_caps_all;
static arch_cpu_caps_record_t g_system_caps_any;
static arch_cpu_caps_record_t g_per_cpu_caps[MAX_CPUS];
static bool g_cpu_caps_present[MAX_CPUS];
static size_t g_cpu_caps_present_count;
static bool g_system_caps_finalized;

static void hal_feature_set_bit(uint64_t *bits, hal_cpu_feature_t feature,
                                bool enabled) {
    if (enabled && feature < HAL_CPU_FEATURE__COUNT) {
        bits[(size_t)feature / 64u] |= 1ULL << ((size_t)feature % 64u);
    }
}

static bool cpu_caps_export_record(const arch_cpu_caps_record_t *caps,
                                   hal_cpu_feature_set_t *out) {
    if (caps == NULL || out == NULL) {
        return false;
    }
    for (size_t i = 0; i < sizeof(*out) / sizeof(uint64_t); ++i) {
        ((uint64_t *)out)[i] = 0U;
    }
#define EXPORT_COMMON(hal_feature, arch_feature)                                  \
    do {                                                                           \
        hal_feature_set_bit(out->raw_bits, hal_feature,                            \
                            arch_cpu_caps_test(&caps->raw, arch_feature));          \
        hal_feature_set_bit(out->usable_bits, hal_feature,                         \
                            arch_cpu_caps_test(&caps->usable, arch_feature));       \
    } while (0)
    EXPORT_COMMON(HAL_CPU_FEATURE_VECTOR, ARCH_CPU_FEAT_COMMON_VECTOR);
    EXPORT_COMMON(HAL_CPU_FEATURE_AES, ARCH_CPU_FEAT_COMMON_AES);
    EXPORT_COMMON(HAL_CPU_FEATURE_SHA, ARCH_CPU_FEAT_COMMON_SHA);
    EXPORT_COMMON(HAL_CPU_FEATURE_PMULL, ARCH_CPU_FEAT_COMMON_PMULL);
    EXPORT_COMMON(HAL_CPU_FEATURE_CRYPTO, ARCH_CPU_FEAT_COMMON_CRYPTO);
    EXPORT_COMMON(HAL_CPU_FEATURE_STRONG_ATOMICS, ARCH_CPU_FEAT_COMMON_STRONG_ATOMICS);
    EXPORT_COMMON(HAL_CPU_FEATURE_FAST_TLB_CTX, ARCH_CPU_FEAT_COMMON_FAST_TLB_CTX);
#undef EXPORT_COMMON
    arch_cpu_caps_export_hal_features(caps, out);
    return true;
}

static bool cpu_caps_export_for_cpu(size_t cpu_id, hal_cpu_feature_set_t *out) {
    return cpu_caps_export_record(arch_cpu_caps_for_cpu(cpu_id), out);
}

static bool cpu_caps_export_current(hal_cpu_feature_set_t *out) {
    return cpu_caps_export_record(arch_cpu_caps_current(), out);
}

static bool cpu_caps_export_system(hal_cpu_feature_scope_t scope,
                                   hal_cpu_feature_set_t *out) {
    return cpu_caps_export_record(scope == HAL_CPU_FEATURE_SCOPE_ANY
                                      ? arch_cpu_caps_system_any()
                                      : arch_cpu_caps_system_all(),
                                  out);
}

static const hal_cpu_feature_provider_t g_cpu_feature_provider = {
    .for_cpu = cpu_caps_export_for_cpu,
    .for_current_cpu = cpu_caps_export_current,
    .for_system = cpu_caps_export_system,
};

static void cpu_caps_record_copy(arch_cpu_caps_record_t *dst,
                                 const arch_cpu_caps_record_t *src) {
    volatile uint64_t *d = (volatile uint64_t *)dst;
    const volatile uint64_t *s = (const volatile uint64_t *)src;
    for (size_t i = 0; i < (sizeof(*dst) / sizeof(uint64_t)); ++i) {
        d[i] = s[i];
    }
}

bool arch_cpu_caps_test(const arch_cpu_caps_t *caps, int feat) {
    if (feat < 0 || feat >= ARCH_CPU_FEAT_TARGET__COUNT) return false;
    return (caps->bits[feat / 64] & (1ULL << (feat % 64))) != 0;
}

void arch_cpu_caps_set(arch_cpu_caps_t *caps, int feat) {
    if (feat >= 0 && feat < ARCH_CPU_FEAT_TARGET__COUNT) {
        caps->bits[feat / 64] |= (1ULL << (feat % 64));
    }
}

void arch_cpu_caps_clear(arch_cpu_caps_t *caps, int feat) {
    if (feat >= 0 && feat < ARCH_CPU_FEAT_TARGET__COUNT) {
        caps->bits[feat / 64] &= ~(1ULL << (feat % 64));
    }
}

void arch_cpu_caps_or(arch_cpu_caps_t *dst, const arch_cpu_caps_t *src) {
    for (size_t i = 0; i < (ARCH_CPU_FEAT_TARGET__COUNT + 63u) / 64u; ++i) {
        dst->bits[i] |= src->bits[i];
    }
}

void arch_cpu_caps_and(arch_cpu_caps_t *dst, const arch_cpu_caps_t *src) {
    for (size_t i = 0; i < (ARCH_CPU_FEAT_TARGET__COUNT + 63u) / 64u; ++i) {
        dst->bits[i] &= src->bits[i];
    }
}

void arch_cpu_caps_zero(arch_cpu_caps_t *caps) {
    memset(caps, 0, sizeof(*caps));
}

void arch_cpu_caps_fill(arch_cpu_caps_t *caps) {
    memset(caps, 0xFF, sizeof(*caps));
}

const arch_cpu_caps_record_t *arch_cpu_caps_boot(void) {
    return &g_boot_cpu_caps;
}

const arch_cpu_caps_record_t *arch_cpu_caps_current(void) {
    unsigned int cpu_id = hal_cpu_get_id();
    if (cpu_id < MAX_CPUS) {
        return &g_per_cpu_caps[cpu_id];
    }
    return &g_boot_cpu_caps;
}

const arch_cpu_caps_record_t *arch_cpu_caps_for_cpu(size_t cpu_index) {
    if (cpu_index < MAX_CPUS && g_cpu_caps_present[cpu_index]) {
        return &g_per_cpu_caps[cpu_index];
    }
    return NULL;
}

const arch_cpu_caps_record_t *arch_cpu_caps_system_all(void) {
    return g_system_caps_finalized ? &g_system_caps_all : NULL;
}

const arch_cpu_caps_record_t *arch_cpu_caps_system_any(void) {
    return g_system_caps_finalized ? &g_system_caps_any : NULL;
}

bool arch_cpu_has(int feat) {
    return arch_cpu_has_system_all(feat);
}

bool arch_cpu_has_system_all(int feat) {
    return g_system_caps_finalized && arch_cpu_caps_test(&g_system_caps_all.usable, feat);
}

bool arch_cpu_has_on(size_t cpu_index, int feat) {
    return arch_cpu_has_cpu(cpu_index, feat);
}

bool arch_cpu_has_cpu(size_t cpu_index, int feat) {
    if (cpu_index < MAX_CPUS && g_cpu_caps_present[cpu_index]) {
        return arch_cpu_caps_test(&g_per_cpu_caps[cpu_index].usable, feat);
    }
    return false;
}

bool arch_cpu_has_system_any(int feat) {
    return g_system_caps_finalized && arch_cpu_caps_test(&g_system_caps_any.usable, feat);
}

bool arch_cpu_has_current(int feat) {
    unsigned int cpu_id = hal_cpu_get_id();
    return arch_cpu_has_cpu(cpu_id, feat);
}

void cpu_caps_state_set_boot(const arch_cpu_caps_record_t *caps) {
    static bool provider_registered;
    if (!provider_registered) {
        provider_registered =
            hal_cpu_features_register_provider(&g_cpu_feature_provider);
    }
    memset(g_cpu_caps_present, 0, sizeof(g_cpu_caps_present));
    g_cpu_caps_present_count = 0;
    g_system_caps_finalized = false;

    cpu_caps_record_copy(&g_boot_cpu_caps, caps);
    cpu_caps_record_copy(&g_per_cpu_caps[0], caps);
    g_cpu_caps_present[0] = true;
    g_cpu_caps_present_count = 1;

    // Initialize system caps to boot caps for now
    cpu_caps_record_copy(&g_system_caps_all, caps);
    cpu_caps_record_copy(&g_system_caps_any, caps);
}

void cpu_caps_state_set_ap(unsigned int cpu_id, const arch_cpu_caps_record_t *caps) {
    if (cpu_id < MAX_CPUS && caps != NULL) {
        cpu_caps_record_copy(&g_per_cpu_caps[cpu_id], caps);
        if (!g_cpu_caps_present[cpu_id]) {
            g_cpu_caps_present[cpu_id] = true;
            g_cpu_caps_present_count++;
        }
    }
}

kstatus_t arch_cpu_caps_system_finalize(void) {
    if (g_system_caps_finalized) {
        return K_ERR_BAD_STATE;
    }
    if (g_cpu_caps_present_count == 0U) {
        return K_ERR_IN_PROGRESS;
    }
    bool initialized = false;
    arch_cpu_caps_zero(&g_system_caps_all.raw);
    arch_cpu_caps_zero(&g_system_caps_all.usable);
    arch_cpu_caps_zero(&g_system_caps_any.raw);
    arch_cpu_caps_zero(&g_system_caps_any.usable);

    for (size_t i = 0; i < MAX_CPUS; ++i) {
        if (!g_cpu_caps_present[i]) {
            continue;
        }
        const arch_cpu_caps_record_t *cpu = &g_per_cpu_caps[i];
        if (!initialized) {
            cpu_caps_record_copy(&g_system_caps_all, cpu);
            cpu_caps_record_copy(&g_system_caps_any, cpu);
            initialized = true;
            continue;
        }
        arch_cpu_caps_and(&g_system_caps_all.raw, &cpu->raw);
        arch_cpu_caps_and(&g_system_caps_all.usable, &cpu->usable);
        arch_cpu_caps_or(&g_system_caps_any.raw, &cpu->raw);
        arch_cpu_caps_or(&g_system_caps_any.usable, &cpu->usable);
    }

    if (!initialized) {
        return K_ERR_IN_PROGRESS;
    }
    g_system_caps_finalized = true;
    return K_OK;
}

size_t cpu_caps_state_online_count(void) {
    return g_cpu_caps_present_count;
}
