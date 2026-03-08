#include <bharat/syscalls.h>

void bharat_exit(int code) {
    // Invoke exit syscall
    (void)code;
    while(1) {}
}

int bharat_write(int fd, const void* buf, size_t count) {
    // Invoke write syscall
    (void)fd;
    (void)buf;
    return (int)count;
}
