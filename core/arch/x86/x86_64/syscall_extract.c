#include "trap/syscall_regs.h"
#include "hal/hal.h"

bool arch_trap_is_syscall(const trap_frame_t *frame) {
    if (!frame) return false;
    // x86_64: currently detecting INT 0x80 from trap_entry.S vectoring.
    // In future, also detect SYSCALL instruction via MSR entry point.
    return (frame->cause == 128 || frame->cause == 0x80U);
}

kstatus_t arch_trap_extract_syscall(const trap_frame_t *frame, bh_syscall_regs_t *out) {
    if (!frame || !out) return K_ERR_INVALID_ARG;

    // x86_64 Syscall ABI:
    // nr:  rax
    // args: rdi, rsi, rdx, r10, r8, r9
    // Note: trap_entry.S maps rax to gpr[0], rdi to gpr[1], etc.
    out->nr     = frame->gpr[0]; // rax
    out->arg[0] = frame->gpr[1]; // rdi
    out->arg[1] = frame->gpr[2]; // rsi
    out->arg[2] = frame->gpr[3]; // rdx
    out->arg[3] = frame->gpr[4]; // r10
    out->arg[4] = frame->gpr[5]; // r8
    out->arg[5] = frame->gpr[6]; // r9

    return K_OK;
}

void arch_trap_set_syscall_return(trap_frame_t *frame, uintptr_t value) {
    if (!frame) return;
    frame->gpr[0] = value; // rax
}
