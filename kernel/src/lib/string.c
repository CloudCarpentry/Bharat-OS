/*
 * kernel/src/lib/string.c — Minimal freestanding string builtins
 *
 * Provides memcpy, memset, memmove — the only libc functions the kernel
 * uses internally. These must exist when compiling without -lc.
 */

#include <stddef.h>

void *memcpy(void *dest, const void *src, size_t n) {
  unsigned char *d = (unsigned char *)dest;
  const unsigned char *s = (const unsigned char *)src;
  while (n--)
    *d++ = *s++;
  return dest;
}

void *memset(void *dest, int c, size_t n) {
  unsigned char *d = (unsigned char *)dest;
  while (n--)
    *d++ = (unsigned char)c;
  return dest;
}

void *memmove(void *dest, const void *src, size_t n) {
  unsigned char *d = (unsigned char *)dest;
  const unsigned char *s = (const unsigned char *)src;
  if (d < s) {
    while (n--)
      *d++ = *s++;
  } else {
    d += n;
    s += n;
    while (n--)
      *--d = *--s;
  }
  return dest;
}
