#include "trap/syscall_regs.h"
#include "hal/hal.h"

#define ARM64_ESR_EC(esr) (((esr) >> 26) & 0x3f)
#define ARM64_EC_SVC64    0x15

bool arch_trap_is_syscall(const trap_frame_t *frame) {
    if (!frame) return false;
    // ARM64: SVC instruction from EL0.
    // Decodes ESR_EL1.EC from the cause field (which stores full ESR on Bharat ARM64).
    /* Bharat-OS currently treats all SVC-from-EL0 exceptions as syscall entry.
     * ISS/SVC immediate is reserved for future ABI versioning/debug use.
     */
    return (ARM64_ESR_EC(frame->cause) == ARM64_EC_SVC64);
}

kstatus_t arch_trap_extract_syscall(const trap_frame_t *frame, bh_syscall_regs_t *out) {
    if (!frame || !out) return K_ERR_INVALID_ARG;

    // ARM64 Syscall ABI (Standard SMCCC/Bharat convention):
    // nr:   x8
    // args: x0, x1, x2, x3, x4, x5
    // Note: trap_entry.S maps x0..x30 to gpr[0..30].
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
