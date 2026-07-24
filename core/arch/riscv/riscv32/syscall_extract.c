#include "trap/syscall_regs.h"
#include "kernel/status.h"

/**
 * riscv32 Syscall ABI:
 * - syscall instruction: ecall
 * - syscall number: a7 (x17)
 * - args: a0-a5 (x10-x15)
 * - return: a0 (x10)
 */

bool arch_trap_is_syscall(const trap_frame_t *frame) {
    if (!frame) return false;
    // On RISC-V, syscall is an ECALL instruction which usually has a specific cause
    // 8: Environment call from U-mode
    return (frame->cause == 8);
}

kstatus_t arch_trap_extract_syscall(const trap_frame_t *frame, bh_syscall_regs_t *out) {
    if (!frame || !out) return K_ERR_INVALID_ARG;

    out->nr = frame->gpr[17];
    out->arg[0] = frame->gpr[10];
    out->arg[1] = frame->gpr[11];
    out->arg[2] = frame->gpr[12];
    out->arg[3] = frame->gpr[13];
    out->arg[4] = frame->gpr[14];
    out->arg[5] = frame->gpr[15];

    return K_OK;
}

void arch_trap_set_syscall_return(trap_frame_t *frame, uintptr_t value) {
    if (!frame) return;
    frame->gpr[10] = value;
}
