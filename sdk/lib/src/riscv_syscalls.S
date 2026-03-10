.global bharat_syscall

// long bharat_syscall(long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);
bharat_syscall:
    // a0 = sysno
    // a1 = arg1
    // a2 = arg2
    // a3 = arg3
    // a4 = arg4
    // a5 = arg5
    // a6 = arg6

    // RISC-V syscall convention: a7 = sysno, a0-a5 = args
    mv a7, a0
    mv a0, a1
    mv a1, a2
    mv a2, a3
    mv a3, a4
    mv a4, a5
    mv a5, a6

    ecall
    ret
