#include <bharat/syscalls.h>
#include <stddef.h>

extern long bharat_syscall(long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);

void bharat_exit(int code) {
    bharat_syscall(60, code, 0, 0, 0, 0, 0);
    while(1) {}
}

int bharat_write(int fd, const void* buf, size_t count) {
    return (int)bharat_syscall(1, fd, (long)buf, (long)count, 0, 0, 0);
}

int bharat_read(int fd, void* buf, size_t count) {
    return (int)bharat_syscall(0, fd, (long)buf, (long)count, 0, 0, 0);
}

int bharat_get_subsystem_caps(uint32_t* storage_caps, uint32_t* network_caps) {
    if (storage_caps) {
        *storage_caps = 0U;
    }
    if (network_caps) {
        *network_caps = 0U;
    }
    // Using a dummy syscall number for caps, assuming it maps to a real one eventually
    return (int)bharat_syscall(1001, (long)storage_caps, (long)network_caps, 0, 0, 0, 0);
}
