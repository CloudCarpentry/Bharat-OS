#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file string.h
 * @brief Minimal freestanding memory and string operations for the kernel.
 *
 * This header provides standard POSIX-like memory and string operations
 * (e.g., memset, memcpy, memcmp) for use in generic kernel code.
 *
 * For architecture-specific hardware-accelerated memory operations
 * or strictly scalar fallbacks (e.g., early boot, TLB, IRQ contexts),
 * you MUST use <arch/memops.h> (arch_memset, arch_memset_scalar, etc.).
 */

void *memset(void *dest, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);

int memcmp(const void *s1, const void *s2, size_t n);

size_t strlen(const char *s);
char *strncpy(char *dest, const char *src, size_t n);
int strncmp(const char *s1, const char *s2, size_t n);

int strcmp(const char *s1, const char *s2);
char *strcpy(char *dest, const char *src);

void secure_memzero(void *ptr, size_t len);

#ifdef __cplusplus
}
#endif
