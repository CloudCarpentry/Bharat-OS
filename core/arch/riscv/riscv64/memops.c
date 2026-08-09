/*
 * kernel/src/arch/riscv64/memops.c
 *
 * Dispatcher logic for memory operations on RISC-V 64-bit architectures.
 * Safely delegates to pure integer unrolled loops using XLEN loads/stores.
 */

#include "hal/hal_memops.h"
#include <stddef.h>
#include <stdint.h>

void *hal_memcpy_gpr_bulk(void *dst, const void *src, size_t n);
void *hal_memset_gpr_bulk(void *dst, int c, size_t n);

void *hal_memcpy(void *dst, const void *src, size_t n, uint32_t flags) {
    if (flags & BH_MEMCTX_F_EARLY_BOOT || flags & BH_MEMCTX_F_IRQ_SAFE) {
        return hal_memcpy_scalar(dst, src, n);
    }

    // Fast path using standard GPR loads and stores unrolled loops.
    return hal_memcpy_gpr_bulk(dst, src, n);
}

void *hal_memset(void *dst, int c, size_t n, uint32_t flags) {
    if (flags & BH_MEMCTX_F_EARLY_BOOT || flags & BH_MEMCTX_F_IRQ_SAFE) {
        return hal_memset_scalar(dst, c, n);
    }

    // Fast path using standard GPR loads and stores unrolled loops.
    return hal_memset_gpr_bulk(dst, c, n);
}

void *hal_memmove(void *dst, const void *src, size_t n, uint32_t flags) {
    // For now, memmove defers to scalar conservative overlap handling.
    return hal_memmove_scalar(dst, src, n);
}
