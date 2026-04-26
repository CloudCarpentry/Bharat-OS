#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "trap.h"
#include "trap/syscall_regs.h"
#include "kernel/status.h"

// Mock the arch implementation for RISC-V
bool arch_trap_is_syscall(const trap_frame_t *frame) {
    if (!frame) return false;
    return (frame->cause == 8);
}

kstatus_t arch_trap_extract_syscall(const trap_frame_t *frame, bh_syscall_regs_t *out) {
    if (!frame || !out) return K_ERR_INVALID_ARG;
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

void test_extract_riscv64(void) {
    printf("Testing RISC-V syscall extraction...\n");
    trap_frame_t frame = {0};
    frame.cause = 8;
    frame.gpr[16] = 789; // nr
    frame.gpr[9] = 20;   // arg0
    frame.gpr[10] = 21;
    frame.gpr[11] = 22;
    frame.gpr[12] = 23;
    frame.gpr[13] = 24;
    frame.gpr[14] = 25;  // arg5

    assert(arch_trap_is_syscall(&frame));

    bh_syscall_regs_t regs = {0};
    assert(arch_trap_extract_syscall(&frame, &regs) == K_OK);
    assert(regs.nr == 789);
    assert(regs.arg[0] == 20);
    assert(regs.arg[1] == 21);
    assert(regs.arg[2] == 22);
    assert(regs.arg[3] == 23);
    assert(regs.arg[4] == 24);
    assert(regs.arg[5] == 25);

    arch_trap_set_syscall_return(&frame, 0x777);
    assert(frame.gpr[9] == 0x777);
    printf("RISC-V extraction tests passed.\n");
}

int main(void) {
    test_extract_riscv64();
    return 0;
}
