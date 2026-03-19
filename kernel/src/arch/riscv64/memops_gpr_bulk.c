/*
 * kernel/src/arch/riscv64/memops_gpr_bulk.c
 *
 * Provides optimized integer-only memory operations on RISC-V 64-bit
 * using unrolled loops with XLEN loads and stores, avoiding RVV vector
 * instructions completely to ensure kernel safety.
 */

#include "arch/memops.h"
#include <stdint.h>
#include <stddef.h>

void *arch_memcpy_gpr_bulk(void *dst, const void *src, size_t n) {
    uint8_t *d = dst;
    const uint8_t *s = src;

    // Small copy early exit
    if (n < 8) {
        while (n--) *d++ = *s++;
        return dst;
    }

    // Unaligned heads
    while (((uintptr_t)d & 7) && n > 0) {
        *d++ = *s++;
        n--;
    }

    // XLEN aligned body (only if src is also aligned)
    if (((uintptr_t)s & 7) == 0) {
        uint64_t *d64 = (uint64_t *)d;
        const uint64_t *s64 = (const uint64_t *)s;

        while (n >= 32) {
            uint64_t v0 = s64[0];
            uint64_t v1 = s64[1];
            uint64_t v2 = s64[2];
            uint64_t v3 = s64[3];

            d64[0] = v0;
            d64[1] = v1;
            d64[2] = v2;
            d64[3] = v3;

            d64 += 4;
            s64 += 4;
            n -= 32;
        }

        while (n >= 8) {
            *d64++ = *s64++;
            n -= 8;
        }

        d = (uint8_t *)d64;
        s = (const uint8_t *)s64;
    }

    // Tail (or unaligned fallback)
    while (n--) {
        *d++ = *s++;
    }

    return dst;
}

void *arch_memset_gpr_bulk(void *dst, int c, size_t n) {
    uint8_t *d = dst;
    uint8_t v = (uint8_t)c;

    // Small copy early exit
    if (n < 8) {
        while (n--) *d++ = v;
        return dst;
    }

    // Unaligned heads
    while (((uintptr_t)d & 7) && n > 0) {
        *d++ = v;
        n--;
    }

    uint64_t v64 = v;
    v64 |= v64 << 8;
    v64 |= v64 << 16;
    v64 |= v64 << 32;

    uint64_t *d64 = (uint64_t *)d;

    while (n >= 32) {
        d64[0] = v64;
        d64[1] = v64;
        d64[2] = v64;
        d64[3] = v64;

        d64 += 4;
        n -= 32;
    }

    while (n >= 8) {
        *d64++ = v64;
        n -= 8;
    }

    // Tail
    d = (uint8_t *)d64;
    while (n--) {
        *d++ = v;
    }

    return dst;
}
