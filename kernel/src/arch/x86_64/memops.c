/*
 * kernel/src/arch/x86_64/memops.c
 *
 * Dispatcher logic for memory operations on x86_64 architectures.
 * This determines the safest approach (fast paths with rep movsb
 * vs the purely scalar fallback) depending on the execution context.
 */

#include "arch/memops.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

void *arch_memcpy_fast_string(void *dst, const void *src, size_t n);
void *arch_memset_fast_string(void *dst, int c, size_t n);

void *arch_memcpy(void *dst, const void *src, size_t n, uint32_t flags) {
    if (flags & ARCH_MEMOP_F_EARLY_BOOT || flags & ARCH_MEMOP_F_IRQ_SAFE) {
        return arch_memcpy_scalar(dst, src, n);
    }

    // For x86_64, the rep movsb path is integer-only and very efficient
    // on modern CPUs with ERMS (Enhanced REP MOVSB).
    return arch_memcpy_fast_string(dst, src, n);
}

void *arch_memset(void *dst, int c, size_t n, uint32_t flags) {
    if (flags & ARCH_MEMOP_F_EARLY_BOOT || flags & ARCH_MEMOP_F_IRQ_SAFE) {
        return arch_memset_scalar(dst, c, n);
    }

    // For x86_64, the rep stosb path is integer-only.
    return arch_memset_fast_string(dst, c, n);
}

void *arch_memmove(void *dst, const void *src, size_t n, uint32_t flags) {
    // For now, memmove defers to scalar conservative overlap handling
    return arch_memmove_scalar(dst, src, n);
}
