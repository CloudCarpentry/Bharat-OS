#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "trap.h"
#include "trap/syscall_regs.h"
#include "trap/syscall_context.h"
#include "kernel/status.h"

void x86_syscall_validate_return(bh_syscall_return_context_t *ret);

static void test_return_validation(void) {
    bh_syscall_return_context_t ret = {
        .pc = 0x400000,
        .sp = 0x7fffffffe000,
        .status = 0x202,
        .result = 17,
        .origin = TRAP_ORIGIN_USER,
        .flags = 0,
        .disposition = BH_SYSCALL_RETURN_USER,
    };

    x86_syscall_validate_return(&ret);
    assert(ret.disposition == BH_SYSCALL_RETURN_USER);

    const uintptr_t bad_pc = 0x0000800000000000ULL;
    const uintptr_t original_sp = ret.sp;
    const uintptr_t original_status = ret.status;
    ret.pc = bad_pc;
    x86_syscall_validate_return(&ret);
    assert(ret.disposition == BH_SYSCALL_RETURN_FAULT);
    assert(ret.pc == bad_pc);
    assert(ret.sp == original_sp);
    assert(ret.status == original_status);
}

void test_extract_x86_64(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("Testing x86_64 syscall extraction (INT 0x80)...\n");
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
    kstatus_t status_080 = arch_trap_extract_syscall(&frame, &regs);
    printf("INT 0x80 status = %d, regs.nr = %lu, frame.gpr[0] = %lu\n", status_080, (unsigned long)regs.nr, (unsigned long)frame.gpr[0]);
    assert(status_080 == K_OK);
    assert(regs.nr == 123);
    assert(regs.arg[0] == 1);
    assert(regs.arg[1] == 2);
    assert(regs.arg[2] == 3);
    assert(regs.arg[3] == 4);
    assert(regs.arg[4] == 5);
    assert(regs.arg[5] == 6);

    arch_trap_set_syscall_return(&frame, 0xABC);
    assert(frame.gpr[0] == 0xABC);
    printf("x86_64 INT 0x80 extraction tests passed.\n");

    printf("Testing x86_64 syscall extraction (SYSCALL)...\n");
    frame.cause = 0x100;
    frame.gpr[0] = 123;
    regs.nr = 0; // Clear it
    assert(arch_trap_is_syscall(&frame));
    kstatus_t status = arch_trap_extract_syscall(&frame, &regs);
    printf("status = %d, regs.nr = %lu, frame.gpr[0] = %lu\n", status, (unsigned long)regs.nr, (unsigned long)frame.gpr[0]);
    assert(status == K_OK);
    assert(regs.nr == 123);
    assert(regs.arg[0] == 1);
    printf("x86_64 SYSCALL extraction tests passed.\n");
}

int main(void) {
    test_extract_x86_64();
    test_return_validation();
    return 0;
}
