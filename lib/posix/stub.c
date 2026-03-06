#include "unistd.h"

// Stub file to satisfy library compilation for now
long syscall(long number, ...) {
    (void)number;
    return -1; // Not implemented
}

void _exit(int status) {
    (void)status;
    while(1);
}
