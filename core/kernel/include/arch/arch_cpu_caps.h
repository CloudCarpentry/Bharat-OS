#ifndef BHARAT_ARCH_CPU_CAPS_H
#define BHARAT_ARCH_CPU_CAPS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum {
    /* Common semantic features: portable meaning across ISAs */
    ARCH_CPU_FEAT_COMMON_AES = 0,
    ARCH_CPU_FEAT_COMMON_SHA,
    ARCH_CPU_FEAT_COMMON_PMULL,
    ARCH_CPU_FEAT_COMMON_VECTOR,
    ARCH_CPU_FEAT_COMMON_CRYPTO,
    ARCH_CPU_FEAT_COMMON_FAST_TLB_CTX,
    ARCH_CPU_FEAT_COMMON_STRONG_ATOMICS,

    ARCH_CPU_FEAT_COMMON__COUNT = 32,

    /* Arch-private window starts here */
    ARCH_CPU_FEAT_ARCH_BASE = 32
} arch_cpu_common_feature_t;

// Include arch-specific feature header
#if defined(__x86_64__)
// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
#include "arch/x86_64/cpu_features.h"
#elif defined(__aarch64__)
// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
#include "arch/arm64/cpu_features.h"
#elif defined(__riscv) && defined(BHARAT_ARCH_SHAKTI)
// Distinguish generic riscv from shakti if needed, otherwise fall through
// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
#include "arch/shakti/cpu_features.h"
#elif defined(__riscv)
// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
#include "arch/riscv64/cpu_features.h"
#else
// Fallback for unknown/stub
#define ARCH_CPU_FEAT_TARGET__COUNT ARCH_CPU_FEAT_COMMON__COUNT
#endif

// To handle any missing TARGET count in weird configs
#ifndef ARCH_CPU_FEAT_TARGET__COUNT
#define ARCH_CPU_FEAT_TARGET__COUNT ARCH_CPU_FEAT_COMMON__COUNT
#endif

typedef struct {
    uint64_t bits[(ARCH_CPU_FEAT_TARGET__COUNT + 63u) / 64u];
} arch_cpu_caps_t;

typedef struct {
    arch_cpu_caps_t raw;    // Hardware/Firmware exposed capabilities
    arch_cpu_caps_t usable; // Validated and policy-approved capabilities
} arch_cpu_caps_record_t;

void arch_cpu_caps_init(void);
void arch_cpu_caps_init_ap(void); // For APs
void arch_cpu_caps_system_finalize(void); // To calculate system_all and system_any

const arch_cpu_caps_record_t *arch_cpu_caps_boot(void);
const arch_cpu_caps_record_t *arch_cpu_caps_current(void);
const arch_cpu_caps_record_t *arch_cpu_caps_for_cpu(size_t cpu_index);
const arch_cpu_caps_record_t *arch_cpu_caps_system_all(void);
const arch_cpu_caps_record_t *arch_cpu_caps_system_any(void);

// Scoped helpers for usable feature checks.
bool arch_cpu_has_system_all(int feat);
bool arch_cpu_has_system_any(int feat);
bool arch_cpu_has_current(int feat);
bool arch_cpu_has_cpu(size_t cpu_index, int feat);

// Legacy helper names (kept for compatibility).
// arch_cpu_has() == arch_cpu_has_system_all()
bool arch_cpu_has(int feat);
// arch_cpu_has_on() == arch_cpu_has_cpu()
bool arch_cpu_has_on(size_t cpu_index, int feat);
// Helper to query a specific bitset
bool arch_cpu_caps_test(const arch_cpu_caps_t *caps, int feat);

void arch_cpu_caps_set(arch_cpu_caps_t *caps, int feat);
void arch_cpu_caps_clear(arch_cpu_caps_t *caps, int feat);
void arch_cpu_caps_or(arch_cpu_caps_t *dst, const arch_cpu_caps_t *src);
void arch_cpu_caps_and(arch_cpu_caps_t *dst, const arch_cpu_caps_t *src);
void arch_cpu_caps_zero(arch_cpu_caps_t *caps);
void arch_cpu_caps_fill(arch_cpu_caps_t *caps);

#endif // BHARAT_ARCH_CPU_CAPS_H
