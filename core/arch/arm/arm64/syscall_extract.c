#include "trap/syscall_regs.h"
#include "hal/hal.h"

bool arch_trap_is_syscall(const trap_frame_t *frame) {
    if (!frame) return false;
    // ARM64: SVC instruction from EL0.
    // This usually comes as EC=0x15 in ESR_EL1.
    // For now, check if cause (which trap_entry.S might set) is SVC.
    // If trap_entry.S uses a specific cause for SVC, we check it here.
    return (frame->cause == 0x15);
}

kstatus_t arch_trap_extract_syscall(const trap_frame_t *frame, bh_syscall_regs_t *out) {
    if (!frame || !out) return K_ERR_INVALID_ARG;

    // ARM64 Syscall ABI:
    // nr:  x8
    // args: x0, x1, x2, x3, x4, x5
    // Need to verify how trap_entry.S maps registers.
    // Usually x0-x30 are gpr[0-30].
    out->nr     = frame->gpr[8];
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
    frame->gpr[0] = value; // x0
}
