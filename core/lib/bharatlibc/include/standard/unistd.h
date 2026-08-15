#ifndef BHARATLIBC_UNISTD_H
#define BHARATLIBC_UNISTD_H

#include <standard/stddef.h>
#include <standard/sys/types.h>

ssize_t read(int descriptor, void *buffer, size_t capacity);
ssize_t write(int descriptor, const void *buffer, size_t length);
int close(int descriptor);
pid_t getpid(void);
int isatty(int descriptor);

#endif /* BHARATLIBC_UNISTD_H */
