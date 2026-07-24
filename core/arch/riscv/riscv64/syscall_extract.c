#include "trap/syscall_regs.h"
#include "hal/hal.h"

bool arch_trap_is_syscall(const trap_frame_t *frame) {
    if (!frame) return false;
    // RISC-V: Environment call from U-mode (cause 8).
    return (frame->cause == 8);
}

kstatus_t arch_trap_extract_syscall(const trap_frame_t *frame, bh_syscall_regs_t *out) {
    if (!frame || !out) return K_ERR_INVALID_ARG;

    // RISC-V Syscall ABI:
    // nr:  a7 (x17)
    // args: a0-a5 (x10-x15)
    // trap_entry.S mapping:
    // x1  -> gpr[0]
    // x2  -> gpr[1]
    // ...
    // x10 -> gpr[9]
    // x17 -> gpr[16]
    out->nr     = frame->gpr[16]; // a7 (x17)
    out->arg[0] = frame->gpr[9];  // a0 (x10)
    out->arg[1] = frame->gpr[10]; // a1 (x11)
    out->arg[2] = frame->gpr[11]; // a2 (x12)
    out->arg[3] = frame->gpr[12]; // a3 (x13)
    out->arg[4] = frame->gpr[13]; // a4 (x14)
    out->arg[5] = frame->gpr[14]; // a5 (x15)

    return K_OK;
}

void arch_trap_set_syscall_return(trap_frame_t *frame, uintptr_t value) {
    if (!frame) return;
    frame->gpr[9] = value; // a0 (x10)
}
