#include <assert.h>
#include <stdio.h>
#include "trap/syscall_regs.h"
#include "hal/hal.h"

// Mock trap_frame_t for testing
// Note: We need to match the architecture's trap_frame_t structure.
// This is a host test, so we'll provide a generic one if not defined.
#ifndef TRAP_FRAME_DEFINED
typedef struct trap_frame {
    uint64_t gpr[32];
    uint64_t pc;
    uint64_t sp;
    uint64_t cause;
    uint32_t from_user;
} trap_frame_t;
#define TRAP_FRAME_DEFINED
#endif

extern bool arch_trap_is_syscall(const trap_frame_t *frame);
extern kstatus_t arch_trap_extract_syscall(const trap_frame_t *frame, bh_syscall_regs_t *out);

void test_x86_64_extraction(void) {
    trap_frame_t frame = {0};
    bh_syscall_regs_t regs = {0};

    frame.cause = 0x80;
    frame.gpr[0] = 100; // nr
    frame.gpr[1] = 1;   // arg0
    frame.gpr[2] = 2;   // arg1
    frame.gpr[3] = 3;   // arg2
    frame.gpr[4] = 4;   // arg3
    frame.gpr[5] = 5;   // arg4
    frame.gpr[6] = 6;   // arg5

    assert(arch_trap_is_syscall(&frame) == true);
    assert(arch_trap_extract_syscall(&frame, &regs) == K_OK);
    assert(regs.nr == 100);
    assert(regs.arg[0] == 1);
    assert(regs.arg[1] == 2);
    assert(regs.arg[2] == 3);
    assert(regs.arg[3] == 4);
    assert(regs.arg[4] == 5);
    assert(regs.arg[5] == 6);
    printf("x86_64 extraction test passed.\n");
}

void test_arm64_extraction(void) {
    // Mocking ARM64 extraction logic if we were on ARM64
    // Since we are compiling arch-specific files, we'll only test the one we link.
}

int main(void) {
#if defined(__x86_64__)
    test_x86_64_extraction();
#endif
    return 0;
}
