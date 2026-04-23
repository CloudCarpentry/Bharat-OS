#include "arch/memops.h"

void *arch_memcpy(void *dst, const void *src, size_t n, uint32_t flags) {
    (void)flags;
    return arch_memcpy_scalar(dst, src, n);
}

void *arch_memset(void *dst, int c, size_t n, uint32_t flags) {
    (void)flags;
    return arch_memset_scalar(dst, c, n);
}

void *arch_memmove(void *dst, const void *src, size_t n, uint32_t flags) {
    (void)flags;
    return arch_memmove_scalar(dst, src, n);
}
