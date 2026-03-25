#ifndef BHARAT_ARCH_BITOPS_H
#define BHARAT_ARCH_BITOPS_H

#include <stdint.h>
#include <stddef.h>

/**
 * @file bitops.h
 * @brief Architecture-specific hardware-accelerated bit manipulation operations.
 *
 * Provides architecture hooks to leverage hardware CLZ/CTZ instructions
 * (e.g., x86 `BSF/BSR`, ARM `CLZ`, RISC-V `CTZ`) for fast bit scanning.
 */

/**
 * @brief Count Leading Zeros.
 * @param x The 64-bit value.
 * @return The number of leading zeros in x. If x is 0, the result is undefined.
 */
static inline int arch_clz64(uint64_t x) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_clzll(x);
#else
    int n = 0;
    if (x == 0) return 64;
    while ((x & (1ULL << 63)) == 0) {
        n++;
        x <<= 1;
    }
    return n;
#endif
}

/**
 * @brief Count Trailing Zeros.
 * @param x The 64-bit value.
 * @return The number of trailing zeros in x. If x is 0, the result is undefined.
 */
static inline int arch_ctz64(uint64_t x) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_ctzll(x);
#else
    int n = 0;
    if (x == 0) return 64;
    while ((x & 1) == 0) {
        n++;
        x >>= 1;
    }
    return n;
#endif
}

/**
 * @brief Count Leading Zeros (32-bit).
 * @param x The 32-bit value.
 * @return The number of leading zeros in x. If x is 0, the result is undefined.
 */
static inline int arch_clz32(uint32_t x) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_clz(x);
#else
    int n = 0;
    if (x == 0) return 32;
    while ((x & (1U << 31)) == 0) {
        n++;
        x <<= 1;
    }
    return n;
#endif
}

/**
 * @brief Count Trailing Zeros (32-bit).
 * @param x The 32-bit value.
 * @return The number of trailing zeros in x. If x is 0, the result is undefined.
 */
static inline int arch_ctz32(uint32_t x) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_ctz(x);
#else
    int n = 0;
    if (x == 0) return 32;
    while ((x & 1) == 0) {
        n++;
        x >>= 1;
    }
    return n;
#endif
}

#endif /* BHARAT_ARCH_BITOPS_H */
