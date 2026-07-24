#ifndef BHARAT_SYSCALL_REGS_H
#define BHARAT_SYSCALL_REGS_H

#include <stdint.h>
#include <stdbool.h>
#include "trap.h"
#include "kernel/status.h"
#include "bh_personality.h"

typedef struct bh_syscall_regs {
    uintptr_t nr;
    uintptr_t arg[6];
} bh_syscall_regs_t;

typedef bh_personality_kind_t bh_personality_id_t;

typedef struct bh_syscall_ctx {
    struct bh_thread *thread;
    struct bh_process *process;
    bh_personality_id_t personality;
    bh_syscall_regs_t regs;
    const struct bh_syscall_meta *desc;
} bh_syscall_ctx_t;

bool arch_trap_is_syscall(const trap_frame_t *frame);
kstatus_t arch_trap_extract_syscall(const trap_frame_t *frame, bh_syscall_regs_t *out);
void arch_trap_set_syscall_return(trap_frame_t *frame, uintptr_t value);

#endif /* BHARAT_SYSCALL_REGS_H */
