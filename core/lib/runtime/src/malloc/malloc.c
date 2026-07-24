#include <stdlib.h>
#include <uapi/syscall/syscall_nr.h>
#include <syscall/common/syscall.h>

/* Minimal slab-like allocator can be implemented here later for better performance.
   For now, we use direct syscalls as a baseline. */

void* malloc(size_t size) {
    uint64_t addr;
    long ret = bh_syscall(SYSCALL_MEM_ALLOC, (long)size, 0, 0, (long)&addr, 0, 0);
    if (ret == 0) {
        return (void*)addr;
    }
    return NULL;
}

void free(void* ptr) {
    if (ptr) {
        bh_syscall(SYSCALL_MEM_FREE, (long)ptr, 0, 0, 0, 0, 0);
    }
}

void exit(int status) {
    bh_syscall(SYSCALL_THREAD_EXIT, (long)status, 0, 0, 0, 0, 0);
    while (1) {}
}

void abort(void) {
    exit(-1);
}
