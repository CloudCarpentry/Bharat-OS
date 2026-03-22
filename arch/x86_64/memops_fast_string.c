/*
 * kernel/src/arch/x86_64/memops_fast_string.c
 *
 * Provides optimized integer-only memory operations using x86 rep movsb
 * and rep stosb instructions, which take advantage of Enhanced REP MOVSB
 * (ERMS) on modern Intel/AMD processors.
 */

#include "arch/memops.h"

void *arch_memcpy_fast_string(void *dst, const void *src, size_t n) {
    void *ret = dst;
    __asm__ __volatile__(
        "rep movsb"
        : "+D"(dst), "+S"(src), "+c"(n)
        :
        : "memory"
    );
    return ret;
}

void *arch_memset_fast_string(void *dst, int c, size_t n) {
    void *ret = dst;
    __asm__ __volatile__(
        "rep stosb"
        : "+D"(dst), "+c"(n)
        : "a"(c)
        : "memory"
    );
    return ret;
}
