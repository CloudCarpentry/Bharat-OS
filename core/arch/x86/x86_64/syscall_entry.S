.extern bh_syscall_gate
.extern sched_current_thread

.section .text
.global x86_64_syscall_entry
.type x86_64_syscall_entry, @function

x86_64_syscall_entry:
    /*
     * SYSCALL instruction:
     * - RCX = user RIP
     * - R11 = user RFLAGS
     * - CS/SS loaded from STAR MSR
     */

    /* Swap GS to get kernel per-CPU area */
    swapgs

    /* Save user stack and load kernel stack from per-CPU area */
    /* For now, simplified: assume per-CPU area has kernel stack at a known offset */
    /* mov %rsp, %gs:USER_SP_OFFSET */
    /* mov %gs:KERNEL_SP_OFFSET, %rsp */

    /* Build trap_frame_t on kernel stack */
    sub $0x80, %rsp /* Space for registers */

    /* Save GPRs */
    mov %rax, 0x00(%rsp)
    mov %rdi, 0x08(%rsp)
    mov %rsi, 0x10(%rsp)
    mov %rdx, 0x18(%rsp)
    mov %rcx, 0x20(%rsp) /* User RIP */
    mov %r8,  0x28(%rsp)
    mov %r9,  0x30(%rsp)
    mov %r10, 0x38(%rsp)
    mov %r11, 0x40(%rsp) /* User RFLAGS */
    /* ... save others if needed for trap_frame_t ... */

    /* Call bh_syscall_gate */
    /* long bh_syscall_gate(trap_frame_t *frame, const trap_info_t *info) */
    /* But trap_dispatch_syscall is the one calling bh_syscall_gate via personality */

    /* Simplified for scaffold: call a C wrapper that sets up trap_info_t */
    mov %rsp, %rdi /* frame */
    /* call x86_64_syscall_handler_c */

    /* Restore state */
    mov 0x40(%rsp), %r11
    mov 0x20(%rsp), %rcx
    mov 0x00(%rsp), %rax
    /* ... restore others ... */

    add $0x80, %rsp

    /* swapgs back */
    swapgs

    sysretq
