/*
 * hal/common/memops/memops_fallback.c
 *
 * Generic portable C implementations of memory operations.
 * These act as the default fallback for architectures that
 * do not provide their own hardware-accelerated paths.
 */

#include <stddef.h>
#include <stdint.h>
#include "arch/memops.h"

void *arch_memcpy(void *dest, const void *src, size_t n, uint32_t flags) {
    (void)flags; /* unused in portable fallback */
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

void *arch_memset(void *dest, int c, size_t n, uint32_t flags) {
    (void)flags; /* unused in portable fallback */
    unsigned char *d = dest;
    while (n--) {
        *d++ = (unsigned char)c;
    }
    return dest;
}

void *arch_memmove(void *dest, const void *src, size_t n, uint32_t flags) {
    (void)flags; /* unused in portable fallback */
    unsigned char *d = dest;
    const unsigned char *s = src;
    if (d < s) {
        while (n--) {
            *d++ = *s++;
        }
    } else {
        d += n;
        s += n;
        while (n--) {
            *(--d) = *(--s);
        }
    }
    return dest;
}
