#include "trap/syscall_regs.h"
#include "kernel/status.h"
#include "trap.h"

bool arch_trap_is_syscall(const trap_frame_t *frame) {
    (void)frame;
    return false;
}

kstatus_t arch_trap_extract_syscall(const trap_frame_t *frame, bh_syscall_regs_t *out) {
    (void)frame;
    (void)out;
    return K_ERR_UNSUPPORTED;
}

void arch_trap_set_syscall_return(trap_frame_t *frame, uintptr_t value) {
    (void)frame;
    (void)value;
}
