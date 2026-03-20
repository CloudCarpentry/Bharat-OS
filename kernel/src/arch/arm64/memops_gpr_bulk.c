/*
 * kernel/src/arch/arm64/memops_gpr_bulk.c
 *
 * Provides optimized integer-only memory operations on ARM64 using
 * bulk GPR operations (ldp/stp). This leverages efficient pairing
 * without touching SIMD or NEON registers.
 */

#include "arch/memops.h"
#include <stdint.h>
#include <stddef.h>

void *arch_memcpy_gpr_bulk(void *dst, const void *src, size_t n) {
    uint8_t *d = dst;
    const uint8_t *s = src;

    // Small copy early exit
    if (n < 16) {
        while (n--) *d++ = *s++;
        return dst;
    }

    // Align d to 16 bytes (if possible without breaking s alignment, or just proceed)
    // For simplicity, we process unaligned up to 16-byte boundary
    while (((uintptr_t)d & 15) && n > 0) {
        *d++ = *s++;
        n--;
    }

    uint64_t *d64 = (uint64_t *)d;
    const uint64_t *s64 = (const uint64_t *)s;

    while (n >= 64) {
        uint64_t x0, x1, x2, x3, x4, x5, x6, x7;
        __asm__ __volatile__(
            "ldp %0, %1, [%8], #16\n"
            "ldp %2, %3, [%8], #16\n"
            "ldp %4, %5, [%8], #16\n"
            "ldp %6, %7, [%8], #16\n"
            "stp %0, %1, [%9], #16\n"
            "stp %2, %3, [%9], #16\n"
            "stp %4, %5, [%9], #16\n"
            "stp %6, %7, [%9], #16\n"
            : "=&r"(x0), "=&r"(x1), "=&r"(x2), "=&r"(x3),
              "=&r"(x4), "=&r"(x5), "=&r"(x6), "=&r"(x7),
              "+r"(s64), "+r"(d64)
            :
            : "memory"
        );
        n -= 64;
    }

    while (n >= 16) {
        uint64_t x0, x1;
        __asm__ __volatile__(
            "ldp %0, %1, [%2], #16\n"
            "stp %0, %1, [%3], #16\n"
            : "=&r"(x0), "=&r"(x1), "+r"(s64), "+r"(d64)
            :
            : "memory"
        );
        n -= 16;
    }

    // Tail
    d = (uint8_t *)d64;
    s = (const uint8_t *)s64;
    while (n--) {
        *d++ = *s++;
    }

    return dst;
}

void *arch_memset_gpr_bulk(void *dst, int c, size_t n) {
    uint8_t *d = dst;
    uint8_t v = (uint8_t)c;

    // Small copy early exit
    if (n < 16) {
        while (n--) *d++ = v;
        return dst;
    }

    // Align d to 16 bytes
    while (((uintptr_t)d & 15) && n > 0) {
        *d++ = v;
        n--;
    }

    uint64_t v64 = v;
    v64 |= v64 << 8;
    v64 |= v64 << 16;
    v64 |= v64 << 32;

    uint64_t *d64 = (uint64_t *)d;

    while (n >= 64) {
        __asm__ __volatile__(
            "stp %1, %1, [%0], #16\n"
            "stp %1, %1, [%0], #16\n"
            "stp %1, %1, [%0], #16\n"
            "stp %1, %1, [%0], #16\n"
            : "+r"(d64)
            : "r"(v64)
            : "memory"
        );
        n -= 64;
    }

    while (n >= 16) {
        __asm__ __volatile__(
            "stp %1, %1, [%0], #16\n"
            : "+r"(d64)
            : "r"(v64)
            : "memory"
        );
        n -= 16;
    }

    // Tail
    d = (uint8_t *)d64;
    while (n--) {
        *d++ = v;
    }

    return dst;
}
