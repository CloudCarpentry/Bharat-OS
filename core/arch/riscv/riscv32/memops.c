/* RV32 dispatcher: use the canonical byte-only fallback until an XLEN-neutral
 * GPR backend is qualified.  This file must not depend on RV64 objects. */

#include "hal/hal_memops.h"

void *hal_memcpy(void *dst, const void *src, size_t n, uint32_t flags) {
    (void)flags;
    return hal_memcpy_scalar(dst, src, n);
}

void *hal_memset(void *dst, int c, size_t n, uint32_t flags) {
    (void)flags;
    return hal_memset_scalar(dst, c, n);
}

void *hal_memmove(void *dst, const void *src, size_t n, uint32_t flags) {
    (void)flags;
    return hal_memmove_scalar(dst, src, n);
}
