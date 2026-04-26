.global bharat_syscall

// long bharat_syscall(long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);
bharat_syscall:
    // x0 = sysno
    // x1 = arg1
    // x2 = arg2
    // x3 = arg3
    // x4 = arg4
    // x5 = arg5
    // x6 = arg6

    // ARM64 syscall convention: x8 = sysno, x0-x5 = args
    mov x8, x0
    mov x0, x1
    mov x1, x2
    mov x2, x3
    mov x3, x4
    mov x4, x5
    mov x5, x6

    svc #0
    ret
