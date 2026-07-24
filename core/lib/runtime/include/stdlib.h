#ifndef BHARAT_SDK_STDLIB_H
#define BHARAT_SDK_STDLIB_H

#include <stddef.h>

void* malloc(size_t size);
void free(void* ptr);
void* calloc(size_t nmemb, size_t size);
void* realloc(void* ptr, size_t size);

void exit(int status);
void abort(void);

#endif /* BHARAT_SDK_STDLIB_H */
