#include "arch/x86_segments.h"
#include "arch/x86_syscall_return.h"

.extern bh_syscall_entry_dispatch
.extern x86_syscall_validate_return
.extern bh_syscall_rejected_return_handoff

.section .text
.global x86_64_syscall_entry
.type x86_64_syscall_entry, @function

# x86_64 cpu_local offsets
.set SELF_OFFSET,        0
.set KERNEL_SP_OFFSET,  8
.set USER_SP_OFFSET,    16

x86_64_syscall_entry:
    /*
     * SYSCALL instruction:
     * - RCX = user RIP
     * - R11 = user RFLAGS
     * - CS/SS loaded from STAR MSR
     * - CPL = 0
     */

    /* Swap GS to get kernel per-CPU area */
    swapgs

    /* Save user stack and load kernel stack from per-CPU area */
    mov %rsp, %gs:USER_SP_OFFSET
    mov %gs:KERNEL_SP_OFFSET, %rsp

    /*
     * Build trap_frame_t on kernel stack.
     * trap_frame_t structure (core/kernel/include/trap.h):
     * uintptr_t gpr[31]; // 0-248
     * uintptr_t sp;      // 248
     * uintptr_t pc;      // 256
     * uintptr_t cause;   // 264
     * uintptr_t status;   // 272
     * uint32_t type;      // 280
     * uint32_t from_user; // 284
     */
    sub $288, %rsp

    /* Save GPRs (map x86_64 registers to trap_frame_t gpr[0..14]) */
    mov %rax, 0(%rsp)     # gpr[0]
    mov %rdi, 8(%rsp)     # gpr[1]
    mov %rsi, 16(%rsp)    # gpr[2]
    mov %rdx, 24(%rsp)    # gpr[3]
    mov %r10, 32(%rsp)    # gpr[4]
    mov %r8,  40(%rsp)    # gpr[5]
    mov %r9,  48(%rsp)    # gpr[6]
    mov %rcx, 56(%rsp)    # gpr[7] (SYSCALL puts user RIP here)
    mov %r11, 64(%rsp)    # gpr[8] (SYSCALL puts user RFLAGS here)
    mov %rbx, 72(%rsp)    # gpr[9]
    mov %rbp, 80(%rsp)    # gpr[10]
    mov %r12, 88(%rsp)    # gpr[11]
    mov %r13, 96(%rsp)    # gpr[12]
    mov %r14, 104(%rsp)   # gpr[13]
    mov %r15, 112(%rsp)   # gpr[14]

    /* pc (User RIP) */
    mov %rcx, 256(%rsp)
    /* sp (User SP) */
    mov %gs:USER_SP_OFFSET, %rax
    mov %rax, 248(%rsp)
    /* cause (Pseudo-vector for SYSCALL) */
    movq $0x100, 264(%rsp)
    /* status (User RFLAGS) */
    mov %r11, 272(%rsp)
    /* type (SYNC = 0) */
    movl $0, 280(%rsp)
    /* from_user (1) */
    movl $1, 284(%rsp)

    /* Allocate bh_syscall_return_context_t on stack (48 bytes: 4x8 + 4 + 4 + 4 + padding)
       Let's align and allocate 48 bytes. */
    sub $X86_SYSCALL_RET_CONTEXT_SIZE, %rsp

    /* Call the fixed-signature C bridge with trap_frame_t * in RDI and return context in RSI. */
    /* RDI = trap_frame_t * */
    lea X86_SYSCALL_RET_CONTEXT_SIZE(%rsp), %rdi
    /* RSI = bh_syscall_return_context_t * */
    mov %rsp, %rsi
    call bh_syscall_entry_dispatch

    /* Call validation routine with return context in RDI */
    mov %rsp, %rdi
    call x86_syscall_validate_return

    /* Only validated USER contexts may restore state and reach SYSRETQ. */
    cmpl $X86_SYSCALL_RETURN_USER_VALUE, X86_SYSCALL_RET_DISPOSITION_OFFSET(%rsp)
    jne .L_syscall_return_rejected

    /* Restore registers from trap_frame_t (which is at +48 from %rsp) */
    mov X86_SYSCALL_RET_CONTEXT_SIZE+8(%rsp), %rdi
    mov X86_SYSCALL_RET_CONTEXT_SIZE+16(%rsp), %rsi
    mov X86_SYSCALL_RET_CONTEXT_SIZE+24(%rsp), %rdx
    mov X86_SYSCALL_RET_CONTEXT_SIZE+32(%rsp), %r10
    mov X86_SYSCALL_RET_CONTEXT_SIZE+40(%rsp), %r8
    mov X86_SYSCALL_RET_CONTEXT_SIZE+48(%rsp), %r9
    mov X86_SYSCALL_RET_CONTEXT_SIZE+72(%rsp), %rbx
    mov X86_SYSCALL_RET_CONTEXT_SIZE+80(%rsp), %rbp
    mov X86_SYSCALL_RET_CONTEXT_SIZE+88(%rsp), %r12
    mov X86_SYSCALL_RET_CONTEXT_SIZE+96(%rsp), %r13
    mov X86_SYSCALL_RET_CONTEXT_SIZE+104(%rsp), %r14
    mov X86_SYSCALL_RET_CONTEXT_SIZE+112(%rsp), %r15

    /* Use authoritative values from bh_syscall_return_context_t for return.
       Offsets in bh_syscall_return_context_t:
       0:  pc
       8:  sp
       16: status
       24: result
       32: origin (4 bytes)
       36: flags (4 bytes)
       40: disposition (4 bytes)
    */
    mov X86_SYSCALL_RET_RESULT_OFFSET(%rsp), %rax
    mov X86_SYSCALL_RET_PC_OFFSET(%rsp), %rcx
    mov X86_SYSCALL_RET_STATUS_OFFSET(%rsp), %r11
    mov X86_SYSCALL_RET_SP_OFFSET(%rsp), %rdx

    /* Save user SP to cpu_local for next entry */
    mov %rdx, %gs:USER_SP_OFFSET

    /* Pop return context and trap frame */
    add $(288+X86_SYSCALL_RET_CONTEXT_SIZE), %rsp

    /* Restore user SP */
    mov %gs:USER_SP_OFFSET, %rsp

    /* swapgs back */
    swapgs

    /*
     * sysretq restores:
     * RIP from RCX
     * RFLAGS from R11
     * CS/SS from STAR MSR
     */
    sysretq

.L_syscall_return_rejected:
    /* Still CPL0 with kernel GS active; this call is deliberately noreturn. */
    mov X86_SYSCALL_RET_DISPOSITION_OFFSET(%rsp), %edi
    call bh_syscall_rejected_return_handoff
    ud2
