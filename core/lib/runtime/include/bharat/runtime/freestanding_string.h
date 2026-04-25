#ifndef BHARAT_RUNTIME_FREESTANDING_STRING_H
#define BHARAT_RUNTIME_FREESTANDING_STRING_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file freestanding_string.h
 * @brief Minimal freestanding memory and string operations for user-space services.
 */

void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

size_t strlen(const char *s);
char *strncpy(char *dest, const char *src, size_t n);
int strncmp(const char *s1, const char *s2, size_t n);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_RUNTIME_FREESTANDING_STRING_H
