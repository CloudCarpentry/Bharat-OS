.section .text
.global bharat_syscall

// long bharat_syscall(long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);
bharat_syscall:
    // rdi = sysno
    // rsi = arg1
    // rdx = arg2
    // rcx = arg3
    // r8  = arg4
    // r9  = arg5
    // [stack] = arg6

    // Using legacy INT 0x80 for now as kernel does not yet support SYSCALL/SYSRET MSRs
    movq %rdi, %rax
    movq %rsi, %rdi
    movq %rdx, %rsi
    movq %rcx, %rdx
    movq %r8,  %r10
    movq %r9,  %r8
    movq 8(%rsp), %r9

    int $0x80
    ret
