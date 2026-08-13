#ifndef BHARATLIBC_STDLIB_H
#define BHARATLIBC_STDLIB_H

#include <standard/stddef.h>

void *malloc(size_t size);
void free(void *pointer);
void exit(int status);

#endif /* BHARATLIBC_STDLIB_H */
