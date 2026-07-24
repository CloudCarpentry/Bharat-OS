#ifndef BHARAT_KERNEL_DS_BH_CPUMASK_H
#define BHARAT_KERNEL_DS_BH_CPUMASK_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/status.h"

/**
 * @file bh_cpumask.h
 * @brief Canonical fixed-size CPU mask for Bharat-OS.
 *
 * This primitive provides a fixed-size bitmask for representing sets of CPUs.
 * It is designed for kernel use in schedulers, TLB shootdowns, and IPIs.
 *
 * Design Constraints:
 * - Fixed-size (compile-time bounded).
 * - No heap allocation.
 * - Non-atomic (caller must synchronize if shared).
 * - Optimized for 64-bit words.
 */

#ifndef BHARAT_MAX_CPUS
#ifdef BHARAT_MAX_CORES
#define BHARAT_MAX_CPUS BHARAT_MAX_CORES
#else
#define BHARAT_MAX_CPUS 64
#endif
#endif

#define BH_CPUMASK_BITS_PER_WORD 64
#define BH_CPUMASK_WORDS \
    ((BHARAT_MAX_CPUS + BH_CPUMASK_BITS_PER_WORD - 1) / BH_CPUMASK_BITS_PER_WORD)

typedef struct {
    uint64_t bits[BH_CPUMASK_WORDS];
} bh_cpumask_t;

/**
 * @brief Check if a CPU index is valid for the current configuration.
 */
static inline bool bh_cpumask_cpu_valid(uint32_t cpu) {
    return cpu < BHARAT_MAX_CPUS;
}

/* ── Unchecked Hot-Path Operations ───────────────────────────────────────── */

void bh_cpumask_zero(bh_cpumask_t *mask);
void bh_cpumask_fill(bh_cpumask_t *mask);
void bh_cpumask_set(bh_cpumask_t *mask, uint32_t cpu);
void bh_cpumask_clear(bh_cpumask_t *mask, uint32_t cpu);
bool bh_cpumask_test(const bh_cpumask_t *mask, uint32_t cpu);
bool bh_cpumask_empty(const bh_cpumask_t *mask);
void bh_cpumask_copy(bh_cpumask_t *dst, const bh_cpumask_t *src);
uint32_t bh_cpumask_weight(const bh_cpumask_t *mask);
int32_t bh_cpumask_first(const bh_cpumask_t *mask);
int32_t bh_cpumask_next(const bh_cpumask_t *mask, uint32_t prev);

void bh_cpumask_and(bh_cpumask_t *dst, const bh_cpumask_t *a, const bh_cpumask_t *b);
void bh_cpumask_or(bh_cpumask_t *dst, const bh_cpumask_t *a, const bh_cpumask_t *b);
void bh_cpumask_xor(bh_cpumask_t *dst, const bh_cpumask_t *a, const bh_cpumask_t *b);
void bh_cpumask_andnot(bh_cpumask_t *dst, const bh_cpumask_t *a, const bh_cpumask_t *b);

bool bh_cpumask_equal(const bh_cpumask_t *a, const bh_cpumask_t *b);
bool bh_cpumask_intersects(const bh_cpumask_t *a, const bh_cpumask_t *b);

/* ── Checked Operations ─────────────────────────────────────────────────── */

kstatus_t bh_cpumask_set_checked(bh_cpumask_t *mask, uint32_t cpu);
kstatus_t bh_cpumask_clear_checked(bh_cpumask_t *mask, uint32_t cpu);
kstatus_t bh_cpumask_test_checked(const bh_cpumask_t *mask, uint32_t cpu, bool *out);

#endif // BHARAT_KERNEL_DS_BH_CPUMASK_H
