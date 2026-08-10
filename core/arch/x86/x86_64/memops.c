/*
 * kernel/src/arch/x86_64/memops.c
 *
 * Dispatcher logic for memory operations on x86_64 architectures.
 * This determines the safest approach (fast paths with rep movsb
 * vs the purely scalar fallback) depending on the execution context.
 */

#include "hal/hal_memops.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

void *hal_memcpy_fast_string(void *dst, const void *src, size_t n);
void *hal_memset_fast_string(void *dst, int c, size_t n);

void *hal_memcpy(void *dst, const void *src, size_t n, uint32_t flags) {
    if (flags & BH_MEMCTX_F_EARLY_BOOT || flags & BH_MEMCTX_F_IRQ_SAFE) {
        return hal_memcpy_scalar(dst, src, n);
    }

    // For x86_64, the rep movsb path is integer-only and very efficient
    // on modern CPUs with ERMS (Enhanced REP MOVSB).
    return hal_memcpy_fast_string(dst, src, n);
}

void *hal_memset(void *dst, int c, size_t n, uint32_t flags) {
    if (flags & BH_MEMCTX_F_EARLY_BOOT || flags & BH_MEMCTX_F_IRQ_SAFE) {
        return hal_memset_scalar(dst, c, n);
    }

    // For x86_64, the rep stosb path is integer-only.
    return hal_memset_fast_string(dst, c, n);
}

void *hal_memmove(void *dst, const void *src, size_t n, uint32_t flags) {
    // For now, memmove defers to scalar conservative overlap handling
    return hal_memmove_scalar(dst, src, n);
}
