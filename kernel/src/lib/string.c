/*
 * kernel/src/lib/string.c — Minimal freestanding string builtins
 *
 * Provides memcpy, memset, memmove — the only libc functions the kernel
 * uses internally. These must exist when compiling without -lc.
 *
 * It delegates purely to the architecture-specific HAL abstractions to
 * ensure hardware accelerators (like rep movsb) are used safely,
 * or purely scalar fallbacks are used in contexts like IRQs and early boot.
 */

#include <stddef.h>
#include <stdint.h>
#include "arch/memops.h"

void *memcpy(void *dest, const void *src, size_t n) {
  return arch_memcpy(dest, src, n, ARCH_MEMOP_F_DEFAULT | ARCH_MEMOP_F_NO_SIMD);
}

void *memset(void *dest, int c, size_t n) {
  return arch_memset(dest, c, n, ARCH_MEMOP_F_DEFAULT | ARCH_MEMOP_F_NO_SIMD);
}

void *memmove(void *dest, const void *src, size_t n) {
  return arch_memmove(dest, src, n, ARCH_MEMOP_F_DEFAULT | ARCH_MEMOP_F_NO_SIMD);
}
