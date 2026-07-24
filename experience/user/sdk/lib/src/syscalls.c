#include <bharat/syscalls.h>
#include <bharat/uapi/syscall_nr.h>
#include <stddef.h>

extern long bharat_syscall(long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);

int bharat_write(int fd, const void* buf, size_t count) {
    return (int)bharat_syscall(SYSCALL_WRITE, fd, (long)buf, (long)count, 0, 0, 0);
}

int bharat_read(int fd, void* buf, size_t count) {
    return (int)bharat_syscall(SYSCALL_READ, fd, (long)buf, (long)count, 0, 0, 0);
}

int bharat_get_subsystem_caps(uint32_t* storage_caps, uint32_t* network_caps) {
    if (storage_caps) {
        *storage_caps = 0U;
    }
    if (network_caps) {
        *network_caps = 0U;
    }
    return (int)bharat_syscall(SYSCALL_GET_SUBSYSTEM_CAPS, (long)storage_caps, (long)network_caps, 0, 0, 0, 0);
}
