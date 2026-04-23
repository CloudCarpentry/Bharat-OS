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
#include "lib/base/string.h"

void *memcpy(void *dest, const void *src, size_t n) {
  return arch_memcpy(dest, src, n, ARCH_MEMOP_F_DEFAULT | ARCH_MEMOP_F_NO_SIMD);
}

void *memset(void *dest, int c, size_t n) {
  return arch_memset(dest, c, n, ARCH_MEMOP_F_DEFAULT | ARCH_MEMOP_F_NO_SIMD);
}

void *memmove(void *dest, const void *src, size_t n) {
  return arch_memmove(dest, src, n, ARCH_MEMOP_F_DEFAULT | ARCH_MEMOP_F_NO_SIMD);
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *p1 = s1;
  const unsigned char *p2 = s2;
  for (size_t i = 0; i < n; i++) {
    if (p1[i] != p2[i]) {
      return (p1[i] < p2[i]) ? -1 : 1;
    }
  }
  return 0;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

char *strcpy(char *dest, const char *src) {
  char *d = dest;
  while ((*d++ = *src++) != '\0') {
    /* copy until null terminator */
  }
  return dest;
}

void secure_memzero(void *ptr, size_t len) {
  volatile unsigned char *p = (volatile unsigned char *)ptr;
  while (len--) {
    *p++ = 0;
  }
}

size_t strlen(const char *s) {
  size_t len = 0;
  while (s[len] != '\0') {
    len++;
  }
  return len;
}

char *strncpy(char *dest, const char *src, size_t n) {
  size_t i;
  for (i = 0; i < n && src[i] != '\0'; i++) {
    dest[i] = src[i];
  }
  for (; i < n; i++) {
    dest[i] = '\0';
  }
  return dest;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  for (size_t i = 0; i < n; i++) {
    if (s1[i] != s2[i] || s1[i] == '\0') {
      return (unsigned char)s1[i] - (unsigned char)s2[i];
    }
  }
  return 0;
}

void __aeabi_memcpy(void *dest, const void *src, size_t n) { memcpy(dest, src, n); }
void __aeabi_memcpy4(void *dest, const void *src, size_t n) { memcpy(dest, src, n); }
void __aeabi_memcpy8(void *dest, const void *src, size_t n) { memcpy(dest, src, n); }
void __aeabi_memclr(void *dest, size_t n) { memset(dest, 0, n); }
void __aeabi_memclr4(void *dest, size_t n) { memset(dest, 0, n); }
void __aeabi_memclr8(void *dest, size_t n) { memset(dest, 0, n); }
void __aeabi_memset(void *dest, size_t n, int c) { memset(dest, c, n); }
void __aeabi_memset4(void *dest, size_t n, int c) { memset(dest, c, n); }
void __aeabi_memset8(void *dest, size_t n, int c) { memset(dest, c, n); }

uint64_t __aeabi_uidivmod(unsigned int n, unsigned int d) {
    if (d == 0) return 0;
    unsigned int q = 0, r = 0;
    for (int i = 31; i >= 0; i--) {
        r <<= 1; r |= (n >> i) & 1;
        if (r >= d) { r -= d; q |= (1U << i); }
    }
    return ((uint64_t)r << 32) | q;
}

unsigned int __aeabi_uidiv(unsigned int n, unsigned int d) {
    return (unsigned int)__aeabi_uidivmod(n, d);
}

float __aeabi_fdiv(float n, float d) { (void)n; (void)d; return 1.0f; }

uint64_t __udivdi3(uint64_t n, uint64_t d) {
    if (d == 0) return 0;
    uint64_t q = 0, r = 0;
    for (int i = 63; i >= 0; i--) {
        r <<= 1; r |= (n >> i) & 1;
        if (r >= d) { r -= d; q |= (1ULL << i); }
    }
    return q;
}
uint64_t __aeabi_uldivmod(uint64_t n, uint64_t d) { return __udivdi3(n, d); }

int __aeabi_fcmplt(float a, float b) { (void)a; (void)b; return 0; }
int __aeabi_fcmpgt(float a, float b) { (void)a; (void)b; return 0; }
float __aeabi_i2f(int a) { (void)a; return 0.0f; }
float __aeabi_fmul(float a, float b) { (void)a; (void)b; return 0.0f; }
int __aeabi_f2iz(float a) { (void)a; return 0; }


