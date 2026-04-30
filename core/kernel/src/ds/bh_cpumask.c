#include "bharat/kernel/ds/bh_cpumask.h"
#include <lib/base/string.h>

void bh_cpumask_zero(bh_cpumask_t *mask) {
    if (!mask) return;
    memset(mask->bits, 0, sizeof(mask->bits));
}

void bh_cpumask_fill(bh_cpumask_t *mask) {
    if (!mask) return;
    memset(mask->bits, 0xFF, sizeof(mask->bits));

    /* Clear tail bits if BHARAT_MAX_CPUS is not a multiple of 64 */
    if (BHARAT_MAX_CPUS % BH_CPUMASK_BITS_PER_WORD) {
        uint32_t last_word = BH_CPUMASK_WORDS - 1;
        uint32_t valid_bits = BHARAT_MAX_CPUS % BH_CPUMASK_BITS_PER_WORD;
        mask->bits[last_word] &= (1ULL << valid_bits) - 1;
    }
}

void bh_cpumask_set(bh_cpumask_t *mask, uint32_t cpu) {
    if (!mask || cpu >= BHARAT_MAX_CPUS) return;
    mask->bits[cpu / BH_CPUMASK_BITS_PER_WORD] |= (1ULL << (cpu % BH_CPUMASK_BITS_PER_WORD));
}

void bh_cpumask_clear(bh_cpumask_t *mask, uint32_t cpu) {
    if (!mask || cpu >= BHARAT_MAX_CPUS) return;
    mask->bits[cpu / BH_CPUMASK_BITS_PER_WORD] &= ~(1ULL << (cpu % BH_CPUMASK_BITS_PER_WORD));
}

bool bh_cpumask_test(const bh_cpumask_t *mask, uint32_t cpu) {
    if (!mask || cpu >= BHARAT_MAX_CPUS) return false;
    return (mask->bits[cpu / BH_CPUMASK_BITS_PER_WORD] & (1ULL << (cpu % BH_CPUMASK_BITS_PER_WORD))) != 0;
}

bool bh_cpumask_empty(const bh_cpumask_t *mask) {
    if (!mask) return true;
    for (uint32_t i = 0; i < BH_CPUMASK_WORDS; i++) {
        if (mask->bits[i] != 0) return false;
    }
    return true;
}

void bh_cpumask_copy(bh_cpumask_t *dst, const bh_cpumask_t *src) {
    if (!dst || !src) return;
    memcpy(dst->bits, src->bits, sizeof(dst->bits));
}

uint32_t bh_cpumask_weight(const bh_cpumask_t *mask) {
    if (!mask) return 0;
    uint32_t weight = 0;
    for (uint32_t i = 0; i < BH_CPUMASK_WORDS; i++) {
        weight += __builtin_popcountll(mask->bits[i]);
    }
    return weight;
}

int32_t bh_cpumask_first(const bh_cpumask_t *mask) {
    if (!mask) return -1;
    for (uint32_t i = 0; i < BH_CPUMASK_WORDS; i++) {
        if (mask->bits[i] != 0) {
            uint32_t bit = __builtin_ctzll(mask->bits[i]);
            uint32_t cpu = i * BH_CPUMASK_BITS_PER_WORD + bit;
            return (cpu < BHARAT_MAX_CPUS) ? (int32_t)cpu : -1;
        }
    }
    return -1;
}

int32_t bh_cpumask_next(const bh_cpumask_t *mask, uint32_t prev) {
    if (!mask) return -1;
    uint32_t next = prev + 1;
    if (next >= BHARAT_MAX_CPUS) return -1;

    uint32_t word_idx = next / BH_CPUMASK_BITS_PER_WORD;
    uint32_t bit_idx = next % BH_CPUMASK_BITS_PER_WORD;

    /* Check current word */
    uint64_t current_word = mask->bits[word_idx] & (~0ULL << bit_idx);
    if (current_word != 0) {
        uint32_t bit = __builtin_ctzll(current_word);
        uint32_t cpu = word_idx * BH_CPUMASK_BITS_PER_WORD + bit;
        return (cpu < BHARAT_MAX_CPUS) ? (int32_t)cpu : -1;
    }

    /* Check remaining words */
    for (uint32_t i = word_idx + 1; i < BH_CPUMASK_WORDS; i++) {
        if (mask->bits[i] != 0) {
            uint32_t bit = __builtin_ctzll(mask->bits[i]);
            uint32_t cpu = i * BH_CPUMASK_BITS_PER_WORD + bit;
            return (cpu < BHARAT_MAX_CPUS) ? (int32_t)cpu : -1;
        }
    }

    return -1;
}

void bh_cpumask_and(bh_cpumask_t *dst, const bh_cpumask_t *a, const bh_cpumask_t *b) {
    if (!dst || !a || !b) return;
    for (uint32_t i = 0; i < BH_CPUMASK_WORDS; i++) {
        dst->bits[i] = a->bits[i] & b->bits[i];
    }
}

void bh_cpumask_or(bh_cpumask_t *dst, const bh_cpumask_t *a, const bh_cpumask_t *b) {
    if (!dst || !a || !b) return;
    for (uint32_t i = 0; i < BH_CPUMASK_WORDS; i++) {
        dst->bits[i] = a->bits[i] | b->bits[i];
    }
}

void bh_cpumask_xor(bh_cpumask_t *dst, const bh_cpumask_t *a, const bh_cpumask_t *b) {
    if (!dst || !a || !b) return;
    for (uint32_t i = 0; i < BH_CPUMASK_WORDS; i++) {
        dst->bits[i] = a->bits[i] ^ b->bits[i];
    }
}

void bh_cpumask_andnot(bh_cpumask_t *dst, const bh_cpumask_t *a, const bh_cpumask_t *b) {
    if (!dst || !a || !b) return;
    for (uint32_t i = 0; i < BH_CPUMASK_WORDS; i++) {
        dst->bits[i] = a->bits[i] & ~b->bits[i];
    }
}

bool bh_cpumask_equal(const bh_cpumask_t *a, const bh_cpumask_t *b) {
    if (!a || !b) return a == b;
    for (uint32_t i = 0; i < BH_CPUMASK_WORDS; i++) {
        if (a->bits[i] != b->bits[i]) return false;
    }
    return true;
}

bool bh_cpumask_intersects(const bh_cpumask_t *a, const bh_cpumask_t *b) {
    if (!a || !b) return false;
    for (uint32_t i = 0; i < BH_CPUMASK_WORDS; i++) {
        if (a->bits[i] & b->bits[i]) return true;
    }
    return false;
}

kstatus_t bh_cpumask_set_checked(bh_cpumask_t *mask, uint32_t cpu) {
    if (!mask) return K_ERR_INVALID_ARG;
    if (!bh_cpumask_cpu_valid(cpu)) return K_ERR_INVALID_ARG;
    bh_cpumask_set(mask, cpu);
    return K_OK;
}

kstatus_t bh_cpumask_clear_checked(bh_cpumask_t *mask, uint32_t cpu) {
    if (!mask) return K_ERR_INVALID_ARG;
    if (!bh_cpumask_cpu_valid(cpu)) return K_ERR_INVALID_ARG;
    bh_cpumask_clear(mask, cpu);
    return K_OK;
}

kstatus_t bh_cpumask_test_checked(const bh_cpumask_t *mask, uint32_t cpu, bool *out) {
    if (!mask || !out) return K_ERR_INVALID_ARG;
    if (!bh_cpumask_cpu_valid(cpu)) return K_ERR_INVALID_ARG;
    *out = bh_cpumask_test(mask, cpu);
    return K_OK;
}
