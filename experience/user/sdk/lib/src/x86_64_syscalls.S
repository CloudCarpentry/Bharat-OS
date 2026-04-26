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

    // x86_64 syscall convention:
    // %rax = sysno
    // %rdi = arg0
    // %rsi = arg1
    // %rdx = arg2
    // %r10 = arg3
    // %r8  = arg4
    // %r9  = arg5

    movq %rdi, %rax
    movq %rsi, %rdi
    movq %rdx, %rsi
    movq %rcx, %rdx
    movq %r8,  %r10
    movq %r9,  %r8
    movq 8(%rsp), %r9

    syscall
    ret
