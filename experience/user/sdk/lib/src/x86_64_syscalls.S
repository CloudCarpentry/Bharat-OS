.section .text
.global bharat_syscall

// long bharat_syscall(long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);
bharat_syscall:
    // User C calling convention (System V):
    // rdi = sysno
    // rsi = arg1
    // rdx = arg2
    // rcx = arg3
    // r8  = arg4
    // r9  = arg5
    // 8(%rsp) = arg6

    // x86_64 SYSCALL convention:
    // rax = sysno
    // rdi = arg1
    // rsi = arg2
    // rdx = arg3
    // r10 = arg4 (Note: syscall instruction clobbers rcx, so we use r10)
    // r8  = arg5
    // r9  = arg6

    movq %rdi, %rax     // sysno -> rax
    movq %rsi, %rdi     // arg1 -> rdi
    movq %rdx, %rsi     // arg2 -> rsi
    movq %rcx, %rdx     // arg3 -> rdx
    movq %r8,  %r10     // arg4 -> r10
    movq %r9,  %r8      // arg5 -> r8
    movq 8(%rsp), %r9   // arg6 -> r9

    syscall
    ret
