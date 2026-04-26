#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "trap.h"
#include "trap/syscall_regs.h"
#include "kernel/status.h"

#define ARM64_ESR_EC(esr) (((esr) >> 26) & 0x3f)
#define ARM64_EC_SVC64    0x15

// Mock the arch implementation for ARM64
bool arch_trap_is_syscall(const trap_frame_t *frame) {
    if (!frame) return false;
    return (ARM64_ESR_EC(frame->cause) == ARM64_EC_SVC64);
}

kstatus_t arch_trap_extract_syscall(const trap_frame_t *frame, bh_syscall_regs_t *out) {
    if (!frame || !out) return K_ERR_INVALID_ARG;
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

void test_extract_arm64(void) {
    printf("Testing ARM64 syscall extraction...\n");
    trap_frame_t frame = {0};
    frame.cause = (ARM64_EC_SVC64 << 26);
    frame.gpr[8] = 456; // nr
    frame.gpr[0] = 10;  // arg0
    frame.gpr[1] = 11;
    frame.gpr[2] = 12;
    frame.gpr[3] = 13;
    frame.gpr[4] = 14;
    frame.gpr[5] = 15;  // arg5

    assert(arch_trap_is_syscall(&frame));

    bh_syscall_regs_t regs = {0};
    assert(arch_trap_extract_syscall(&frame, &regs) == K_OK);
    assert(regs.nr == 456);
    assert(regs.arg[0] == 10);
    assert(regs.arg[1] == 11);
    assert(regs.arg[2] == 12);
    assert(regs.arg[3] == 13);
    assert(regs.arg[4] == 14);
    assert(regs.arg[5] == 15);

    arch_trap_set_syscall_return(&frame, 0xDEF);
    assert(frame.gpr[0] == 0xDEF);
    printf("ARM64 extraction tests passed.\n");
}

int main(void) {
    test_extract_arm64();
    return 0;
}
