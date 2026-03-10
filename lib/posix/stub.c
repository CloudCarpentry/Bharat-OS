#include "unistd.h"

#include <stddef.h>
#include <stdint.h>

// Stub file to satisfy library compilation for now
long syscall(long number, ...) {
    (void)number;
    return -1; // Not implemented
}

void _exit(int status) {
    (void)status;
    while(1);
}

void *memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

void *memcpy(void *dest, const void *src, size_t n) {
    char *d = dest;
    const char *s = src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

uint64_t hal_timer_monotonic_ticks(void) {
    return 0; // Stub
}
