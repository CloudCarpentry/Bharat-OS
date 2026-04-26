#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "trap.h"
#include "trap/syscall_regs.h"
#include "kernel/status.h"

// Mock the arch implementation for x86_64
bool arch_trap_is_syscall(const trap_frame_t *frame) {
    if (!frame) return false;
    return (frame->cause == 128 || frame->cause == 0x80U);
}

kstatus_t arch_trap_extract_syscall(const trap_frame_t *frame, bh_syscall_regs_t *out) {
    if (!frame || !out) return K_ERR_INVALID_ARG;
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

void test_extract_x86_64(void) {
    printf("Testing x86_64 syscall extraction...\n");
    trap_frame_t frame = {0};
    frame.cause = 0x80;
    frame.gpr[0] = 123; // nr
    frame.gpr[1] = 1;   // arg0
    frame.gpr[2] = 2;
    frame.gpr[3] = 3;
    frame.gpr[4] = 4;
    frame.gpr[5] = 5;
    frame.gpr[6] = 6;   // arg5

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
    assert(frame.gpr[0] == 0xABC);
    printf("x86_64 extraction tests passed.\n");
}

int main(void) {
    test_extract_x86_64();
    return 0;
}
