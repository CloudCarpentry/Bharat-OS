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

int bharat_get_subsystem_caps(uint32_t* storage_caps, uint32_t* network_caps) {
    if (storage_caps) {
        *storage_caps = 0U;
    }
    if (network_caps) {
        *network_caps = 0U;
    }
    return -38; // ENOSYS until syscall wire-up lands
}
