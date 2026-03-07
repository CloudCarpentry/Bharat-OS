#ifndef BHARAT_TRAP_H
#define BHARAT_TRAP_H

#include <stdint.h>

#include "sched.h"

typedef enum {
    SYSCALL_NOP = 0,
    SYSCALL_THREAD_CREATE = 1,
    SYSCALL_THREAD_DESTROY = 2,
    SYSCALL_SCHED_YIELD = 3,
    SYSCALL_VMM_MAP_PAGE = 4,
    SYSCALL_VMM_UNMAP_PAGE = 5,
    SYSCALL_CAPABILITY_INVOKE = 6,
    SYSCALL_ENDPOINT_CREATE = 7,
    SYSCALL_ENDPOINT_SEND = 8,
    SYSCALL_ENDPOINT_RECEIVE = 9,
    SYSCALL_CAPABILITY_DELEGATE = 10,
} syscall_id_t;

typedef struct {
    uint64_t gpr[31];
    uint64_t sp;
    uint64_t pc;
    uint64_t cause;
    uint64_t status;
    uint8_t from_user;
} trap_frame_t;

int trap_init(void);
long syscall_dispatch(syscall_id_t id,
                      uint64_t arg0,
                      uint64_t arg1,
                      uint64_t arg2,
                      uint64_t arg3,
                      uint64_t arg4,
                      uint64_t arg5);
long trap_handle(trap_frame_t* frame);

#endif // BHARAT_TRAP_H
