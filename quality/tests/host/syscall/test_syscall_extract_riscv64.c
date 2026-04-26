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
    return (frame->cause == 8); // ecall in RISC-V (from user mode)
}

kstatus_t arch_trap_extract_syscall(const trap_frame_t *frame, bh_syscall_regs_t *out) {
    if (!frame || !out) return K_ERR_INVALID_ARG;
    out->nr     = frame->gpr[17]; // a7
    out->arg[0] = frame->gpr[10]; // a0
    out->arg[1] = frame->gpr[11]; // a1
    out->arg[2] = frame->gpr[12]; // a2
    out->arg[3] = frame->gpr[13]; // a3
    out->arg[4] = frame->gpr[14]; // a4
    out->arg[5] = frame->gpr[15]; // a5
    return K_OK;
}

void arch_trap_set_syscall_return(trap_frame_t *frame, uintptr_t value) {
    if (!frame) return;
    frame->gpr[10] = value; // a0
}

void test_extract_riscv64(void) {
    printf("Testing RISC-V syscall extraction...\n");
    trap_frame_t frame = {0};
    frame.cause = 8;
    frame.gpr[17] = 123; // nr (a7)
    frame.gpr[10] = 1;   // arg0 (a0)
    frame.gpr[11] = 2;
    frame.gpr[12] = 3;
    frame.gpr[13] = 4;
    frame.gpr[14] = 5;
    frame.gpr[15] = 6;   // arg5 (a5)

    assert(arch_trap_is_syscall(&frame));

    bh_syscall_regs_t regs = {0};
    assert(arch_trap_extract_syscall(&frame, &regs) == K_OK);
    assert(regs.nr == 123);
    assert(regs.arg[0] == 1);
    assert(regs.arg[1] == 2);
    assert(regs.arg[2] == 3);
    assert(regs.arg[3] == 4);
    assert(regs.arg[4] == 5);
    assert(regs.arg[5] == 6);

    arch_trap_set_syscall_return(&frame, 0xABC);
    assert(frame.gpr[10] == 0xABC);
    printf("RISC-V extraction tests passed.\n");
}

int main(void) {
    test_extract_riscv64();
    return 0;
}
