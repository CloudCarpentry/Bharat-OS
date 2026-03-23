#ifndef BHARAT_ARCH_MEMOPS_H
#define BHARAT_ARCH_MEMOPS_H

#include <stddef.h>
#include <stdint.h>

/* Execution context and hardware capability flags for memory operations */
#define ARCH_MEMOP_F_DEFAULT        0u
#define ARCH_MEMOP_F_MAY_SLEEP      (1u << 0)
#define ARCH_MEMOP_F_IRQ_SAFE       (1u << 1)
#define ARCH_MEMOP_F_EARLY_BOOT     (1u << 2)
#define ARCH_MEMOP_F_NO_SIMD        (1u << 3)
#define ARCH_MEMOP_F_NO_DMA         (1u << 4)
#define ARCH_MEMOP_F_NO_FAULT       (1u << 5)

/* Architecture-specific dispatched memory operations */
void arch_memops_init(void);
void *arch_memcpy(void *dst, const void *src, size_t n, uint32_t flags);
void *arch_memset(void *dst, int c, size_t n, uint32_t flags);
void *arch_memmove(void *dst, const void *src, size_t n, uint32_t flags);

/* Common safe scalar fallbacks (implemented in common/memops_scalar.c) */
void *arch_memcpy_scalar(void *dst, const void *src, size_t n);
void *arch_memset_scalar(void *dst, int c, size_t n);
void *arch_memmove_scalar(void *dst, const void *src, size_t n);

#endif /* BHARAT_ARCH_MEMOPS_H */
