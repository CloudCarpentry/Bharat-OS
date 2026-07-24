#include "trap/syscall_regs.h"
#include "kernel/status.h"

/**
 * arm32 Syscall ABI:
 * - syscall instruction: svc #0
 * - syscall number: r7
 * - args: r0, r1, r2, r3, r4, r5
 * - return: r0
 */

bool arch_trap_is_syscall(const trap_frame_t *frame) {
    if (!frame) return false;
    // On ARM32, we distinguish syscall by trap type or specific cause
    // This is typically set by the low-level trap entry logic.
    return (frame->type == TRAP_TYPE_SYSCALL);
}

kstatus_t arch_trap_extract_syscall(const trap_frame_t *frame, bh_syscall_regs_t *out) {
    if (!frame || !out) return K_ERR_INVALID_ARG;

    out->nr = frame->gpr[7];
    out->arg[0] = frame->gpr[0];
    out->arg[1] = frame->gpr[1];
    out->arg[2] = frame->gpr[2];
    out->arg[3] = frame->gpr[3];
    out->arg[4] = frame->gpr[4];
    out->arg[5] = frame->gpr[5];

    return K_OK;
}

void arch_trap_set_syscall_return(trap_frame_t *frame, uintptr_t value) {
    if (!frame) return;
    frame->gpr[0] = value;
}
