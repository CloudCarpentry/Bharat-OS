#ifndef BHARAT_ARCH_MEMOPS_H
#define BHARAT_ARCH_MEMOPS_H

#include <stddef.h>
#include <stdint.h>

/*
 * Architecture-specific memory operation flags.
 * Used to dictate the behavior of hardware-accelerated memory routines.
 */
#define ARCH_MEMOP_F_DEFAULT    0x00
#define ARCH_MEMOP_F_NO_SIMD    0x01
#define ARCH_MEMOP_F_DMA_SAFE   0x02

/*
 * Architecture-specific fallback/optimized memory functions.
 * These are implemented by the respective HAL/Arch packages.
 */
void *arch_memcpy(void *dest, const void *src, size_t n, uint32_t flags);
void *arch_memset(void *dest, int c, size_t n, uint32_t flags);
void *arch_memmove(void *dest, const void *src, size_t n, uint32_t flags);

#endif /* BHARAT_ARCH_MEMOPS_H */
