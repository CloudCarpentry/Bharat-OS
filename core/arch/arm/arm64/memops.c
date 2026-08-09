/*
 * kernel/src/arch/arm64/memops.c
 *
 * Dispatcher logic for memory operations on ARM64. It safely delegates
 * to purely integer unrolled loops using ldp/stp, avoiding NEON in
 * sensitive kernel contexts (e.g. IRQs or early boot).
 */

#include "hal/hal_memops.h"
#include <stddef.h>
#include <stdint.h>

void *hal_memcpy_gpr_bulk(void *dst, const void *src, size_t n);
void *hal_memset_gpr_bulk(void *dst, int c, size_t n);

void *hal_memcpy(void *dst, const void *src, size_t n, uint32_t flags) {
    (void)flags;
    /*
     * Keep ARM64 memcpy on the scalar implementation for now.
     *
     * We have observed strict-alignment traps in early boot/selftests on some
     * targets. Until all bulk-copy callers and assembly fast paths are proven
     * alignment-safe under those settings, prefer correctness over throughput.
     */
    return hal_memcpy_scalar(dst, src, n);
}

void *hal_memset(void *dst, int c, size_t n, uint32_t flags) {
    (void)flags;
    return hal_memset_scalar(dst, c, n);
}

void *hal_memmove(void *dst, const void *src, size_t n, uint32_t flags) {
    // For now, memmove defers to scalar conservative overlap handling.
    return hal_memmove_scalar(dst, src, n);
}
