/*
 * kernel/src/arch/riscv64/memops.c
 *
 * Dispatcher logic for memory operations on RISC-V 64-bit architectures.
 * Safely delegates to pure integer unrolled loops using XLEN loads/stores.
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

    // Fast path using standard GPR loads and stores unrolled loops.
    return arch_memcpy_gpr_bulk(dst, src, n);
}

void *arch_memset(void *dst, int c, size_t n, uint32_t flags) {
    if (flags & ARCH_MEMOP_F_EARLY_BOOT || flags & ARCH_MEMOP_F_IRQ_SAFE) {
        return arch_memset_scalar(dst, c, n);
    }

    // Fast path using standard GPR loads and stores unrolled loops.
    return arch_memset_gpr_bulk(dst, c, n);
}

void *arch_memmove(void *dst, const void *src, size_t n, uint32_t flags) {
    // For now, memmove defers to scalar conservative overlap handling.
    return arch_memmove_scalar(dst, src, n);
}
