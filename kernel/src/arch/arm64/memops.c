/*
 * kernel/src/arch/arm64/memops.c
 *
 * Dispatcher logic for memory operations on ARM64. It safely delegates
 * to purely integer unrolled loops using ldp/stp, avoiding NEON in
 * sensitive kernel contexts (e.g. IRQs or early boot).
 */

#include "arch/memops.h"
#include <stddef.h>
#include <stdint.h>

void *arch_memcpy_gpr_bulk(void *dst, const void *src, size_t n);
void *arch_memset_gpr_bulk(void *dst, int c, size_t n);

void *arch_memcpy(void *dst, const void *src, size_t n, uint32_t flags) {
    if (flags & ARCH_MEMOP_F_EARLY_BOOT || flags & ARCH_MEMOP_F_IRQ_SAFE) {
        return arch_memcpy_scalar(dst, src, n);
    }

    // Default fast kernel path for ARM64 using integer loads/stores.
    return arch_memcpy_gpr_bulk(dst, src, n);
}

void *arch_memset(void *dst, int c, size_t n, uint32_t flags) {
    if (flags & ARCH_MEMOP_F_EARLY_BOOT || flags & ARCH_MEMOP_F_IRQ_SAFE) {
        return arch_memset_scalar(dst, c, n);
    }

    // Default fast kernel path for ARM64 using integer stores.
    return arch_memset_gpr_bulk(dst, c, n);
}

void *arch_memmove(void *dst, const void *src, size_t n, uint32_t flags) {
    // For now, memmove defers to scalar conservative overlap handling.
    return arch_memmove_scalar(dst, src, n);
}
