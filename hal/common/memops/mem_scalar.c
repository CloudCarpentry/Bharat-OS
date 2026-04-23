/*
 * hal/common/memops/mem_scalar.c
 *
 * Generic portable C implementations of memory operations.
 * These act as the default fallback for architectures that
 * do not provide their own hardware-accelerated paths.
 */

#include <stddef.h>
#include <stdint.h>
#include "arch/memops.h"
#include "bharat/compiler_safety.h"

BHARAT_NOINLINE BHARAT_USED
void *bharat_memset_scalar(void *dst, int c, size_t n) {
    volatile unsigned char *d = (volatile unsigned char *)dst;
    unsigned char v = (unsigned char)c;
    while (n--) {
        *d++ = v;
    }
    return dst;
}

BHARAT_NOINLINE BHARAT_USED
void *bharat_memcpy_scalar(void *dst, const void *src, size_t n) {
    volatile unsigned char *d = (volatile unsigned char *)dst;
    const volatile unsigned char *s = (const volatile unsigned char *)src;
    while (n--) {
        *d++ = *s++;
    }
    return dst;
}

BHARAT_NOINLINE BHARAT_USED
void *bharat_memmove_scalar(void *dst, const void *src, size_t n) {
    volatile unsigned char *d = (volatile unsigned char *)dst;
    const volatile unsigned char *s = (const volatile unsigned char *)src;

    if (d == s || n == 0) return dst;

    if (d < s) {
        while (n--) {
            *d++ = *s++;
        }
    } else {
        d += n;
        s += n;
        while (n--) {
            *--d = *--s;
        }
    }
    return dst;
}

void *arch_memcpy(void *dest, const void *src, size_t n, uint32_t flags) {
    (void)flags; /* unused in portable fallback */
    return bharat_memcpy_scalar(dest, src, n);
}

void *arch_memset(void *dest, int c, size_t n, uint32_t flags) {
    (void)flags; /* unused in portable fallback */
    return bharat_memset_scalar(dest, c, n);
}

void *arch_memmove(void *dest, const void *src, size_t n, uint32_t flags) {
    (void)flags; /* unused in portable fallback */
    return bharat_memmove_scalar(dest, src, n);
}

void *arch_memcpy_scalar(void *dst, const void *src, size_t n) {
    return bharat_memcpy_scalar(dst, src, n);
}

void *arch_memset_scalar(void *dst, int c, size_t n) {
    return bharat_memset_scalar(dst, c, n);
}

void *arch_memmove_scalar(void *dst, const void *src, size_t n) {
    return bharat_memmove_scalar(dst, src, n);
}
