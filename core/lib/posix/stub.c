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


uint64_t hal_timer_monotonic_ticks(void) {
    return 0; // Stub
}
