.extern bh_syscall_gate
.extern sched_current_thread

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

    /* Call bh_syscall_gate(trap_frame_t *frame) */
    mov %rsp, %rdi
    call bh_syscall_gate

    /* Check if we need to return via sysret or iret */
    /* For production we validate RCX (RIP) and R11 (RFLAGS) */
    mov 256(%rsp), %rcx    /* User RIP */

    /* Simple canonical check: bits 48 to 63 must match bit 47 */
    mov %rcx, %rax
    sar $47, %rax
    inc %rax
    cmp $2, %rax
    jb .L_sysret_safe

    /* Fallback to iretq for non-canonical RIP */
    /* We need to build an iret frame: SS, RSP, RFLAGS, CS, RIP */
    /* Since we've already built a trap_frame_t, we can reuse values */

    /* Restore GPRs first */
    mov 0(%rsp), %rax     # Result
    mov 8(%rsp), %rdi
    mov 16(%rsp), %rsi
    mov 24(%rsp), %rdx
    mov 32(%rsp), %r10
    mov 40(%rsp), %r8
    mov 48(%rsp), %r9
    mov 64(%rsp), %r11    # User RFLAGS
    mov 72(%rsp), %rbx
    mov 80(%rsp), %rbp
    mov 88(%rsp), %r12
    mov 96(%rsp), %r13
    mov 104(%rsp), %r14
    mov 112(%rsp), %r15

    /* Build IRET frame on top of current stack */
    /* trap_frame_t was 288 bytes */
    mov 248(%rsp), %rcx    /* User RSP */
    mov 272(%rsp), %r11    /* User RFLAGS */
    mov 256(%rsp), %rdx    /* User RIP */

    add $288, %rsp

    /* SS is usually 0x1B in Bharat-OS userspace (GDT index 3 | 3) */
    /* CS is usually 0x23 in Bharat-OS userspace (GDT index 4 | 3) */
    pushq $0x1B            /* SS */
    pushq %rcx             /* RSP */
    pushq %r11             /* RFLAGS */
    pushq $0x23            /* CS */
    pushq %rdx             /* RIP */

    swapgs
    iretq

.L_sysret_safe:
    /* Restore registers */
    mov 0(%rsp), %rax     # Result
    mov 8(%rsp), %rdi
    mov 16(%rsp), %rsi
    mov 24(%rsp), %rdx
    mov 32(%rsp), %r10
    mov 40(%rsp), %r8
    mov 48(%rsp), %r9
    mov 56(%rsp), %rcx    # User RIP (if unchanged)
    mov 64(%rsp), %r11    # User RFLAGS (if unchanged)
    mov 72(%rsp), %rbx
    mov 80(%rsp), %rbp
    mov 88(%rsp), %r12
    mov 96(%rsp), %r13
    mov 104(%rsp), %r14
    mov 112(%rsp), %r15

    /* Load user SP back from frame in case it was changed */
    mov 248(%rsp), %rdx
    mov %rdx, %gs:USER_SP_OFFSET

    /* Pop frame */
    add $288, %rsp

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
