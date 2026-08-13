/*
 * kernel/src/arch/x86_64/memops_fast_string.c
 *
 * Provides optimized integer-only memory operations using x86 rep movsb
 * and rep stosb instructions, which take advantage of Enhanced REP MOVSB
 * (ERMS) on modern Intel/AMD processors.
 */

#include "hal/hal_memops.h"

void *hal_memcpy_x86_gpr(void *dst, const void *src, size_t n) {
    unsigned char *d = dst;
    const unsigned char *s = src;
    while (n-- != 0u) *d++ = *s++;
    return dst;
}

void *hal_memset_x86_gpr(void *dst, int c, size_t n) {
    unsigned char *d = dst;
    while (n-- != 0u) *d++ = (unsigned char)c;
    return dst;
}

void *hal_memcpy_fast_string(void *dst, const void *src, size_t n) {
    void *ret = dst;
    __asm__ __volatile__(
        "rep movsb"
        : "+D"(dst), "+S"(src), "+c"(n)
        :
        : "memory"
    );
    return ret;
}

void *hal_memset_fast_string(void *dst, int c, size_t n) {
    void *ret = dst;
    __asm__ __volatile__(
        "rep stosb"
        : "+D"(dst), "+c"(n)
        : "a"(c)
        : "memory"
    );
    return ret;
}

void *hal_memmove_fast_string(void *dst, const void *src, size_t n) {
    void *ret = dst;
    const uintptr_t da = (uintptr_t)dst;
    const uintptr_t sa = (uintptr_t)src;

    if (n == 0u || da == sa) return ret;
    if (da < sa || da - sa >= n) return hal_memcpy_fast_string(dst, src, n);

    unsigned char *d = (unsigned char *)dst + n;
    const unsigned char *s = (const unsigned char *)src + n;
    /* Never expose DF=1 to an interrupt or exception handler. */
    while (n-- != 0u) *--d = *--s;
    return ret;
}

int hal_memcmp_x86_gpr(const void *lhs, const void *rhs, size_t n) {
    const unsigned char *a = lhs;
    const unsigned char *b = rhs;
    while (n-- != 0u) {
        const unsigned av = *a++;
        const unsigned bv = *b++;
        if (av != bv) return (int)av - (int)bv;
    }
    return 0;
}
