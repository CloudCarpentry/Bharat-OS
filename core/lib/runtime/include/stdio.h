#ifndef BHARAT_SDK_STDIO_H
#define BHARAT_SDK_STDIO_H

#ifndef BHARAT_HOST_TEST
#include <stddef.h>
#include <stdarg.h>

int printf(const char* format, ...);
int sprintf(char* str, const char* format, ...);
int snprintf(char* str, size_t size, const char* format, ...);
#endif

#endif /* BHARAT_SDK_STDIO_H */
