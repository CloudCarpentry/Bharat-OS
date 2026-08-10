/*
 * hal/common/memops/mem_scalar.c
 *
 * Canonical Tier-0 memory operations.  These byte-at-a-time routines are the
 * architecture-neutral fallback for IRQ, trap, panic, and early-boot code.
 * They deliberately perform no prefetch, word access, SIMD, DMA, or calls to
 * another memory primitive.  Architecture code exclusively owns hal_mem*().
 */

#include <stddef.h>
#include <stdint.h>
#include "hal/hal_memops.h"

void *hal_memcpy_scalar(void *dst, const void *src, size_t n) {
    volatile unsigned char *d = (volatile unsigned char *)dst;
    const volatile unsigned char *s = (const volatile unsigned char *)src;

    while (n-- != 0u) {
        *d++ = *s++;
    }
    return dst;
}

void *hal_memset_scalar(void *dst, int c, size_t n) {
    volatile unsigned char *d = (volatile unsigned char *)dst;

    while (n-- != 0u) {
        *d++ = (unsigned char)c;
    }
    return dst;
}

void *hal_memmove_scalar(void *dst, const void *src, size_t n) {
    volatile unsigned char *d = (volatile unsigned char *)dst;
    const volatile unsigned char *s = (const volatile unsigned char *)src;
    const uintptr_t da = (uintptr_t)dst;
    const uintptr_t sa = (uintptr_t)src;

    if (n == 0u || da == sa) {
        return dst;
    }

    /* Subtraction is evaluated only in the ordered branch, so it cannot wrap. */
    if (da < sa || da - sa >= n) {
        while (n-- != 0u) {
            *d++ = *s++;
        }
    } else {
        d += n;
        s += n;
        while (n-- != 0u) {
            *--d = *--s;
        }
    }
    return dst;
}

void hal_memset_raw(void *dst, int val, size_t len) {
    volatile unsigned char *d = (volatile unsigned char *)dst;

    while (len-- != 0u) {
        *d++ = (unsigned char)val;
    }
}
