#include "arch/arch_cpu_caps.h"
#include "arch/common/cpu_caps_state.h"
#include "bharat/cpu_local.h"
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
    return &g_system_caps_all;
}

const arch_cpu_caps_record_t *arch_cpu_caps_system_any(void) {
    return &g_system_caps_any;
}

bool arch_cpu_has(int feat) {
    return arch_cpu_has_system_all(feat);
}

bool arch_cpu_has_system_all(int feat) {
    return arch_cpu_caps_test(&g_system_caps_all.usable, feat);
}

bool arch_cpu_has_on(size_t cpu_index, int feat) {
    return arch_cpu_has_cpu(cpu_index, feat);
}

bool arch_cpu_has_cpu(size_t cpu_index, int feat) {
    if (cpu_index < MAX_CPUS) {
        return arch_cpu_caps_test(&g_per_cpu_caps[cpu_index].usable, feat);
    }
    return false;
}

bool arch_cpu_has_system_any(int feat) {
    return arch_cpu_caps_test(&g_system_caps_any.usable, feat);
}

bool arch_cpu_has_current(int feat) {
    unsigned int cpu_id = hal_cpu_get_id();
    return arch_cpu_has_cpu(cpu_id, feat);
}

void cpu_caps_state_set_boot(const arch_cpu_caps_record_t *caps) {
    memset(g_cpu_caps_present, 0, sizeof(g_cpu_caps_present));
    g_cpu_caps_present_count = 0;

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

void arch_cpu_caps_system_finalize(void) {
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
        cpu_caps_record_copy(&g_system_caps_all, &g_boot_cpu_caps);
        cpu_caps_record_copy(&g_system_caps_any, &g_boot_cpu_caps);
    }
}

size_t cpu_caps_state_online_count(void) {
    return g_cpu_caps_present_count;
}
