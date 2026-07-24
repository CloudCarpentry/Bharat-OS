.global bharat_syscall

bharat_syscall:
    /*
     * r7: syscall number (from r0)
     * r0..r5: args (from r1..r2, plus stack args)
     *
     * In ARM EABI, r0-r3 are used for arguments. r4, r5 are next.
     */
    push {r4, r5, r7}
    mov r7, r0
    mov r0, r1
    mov r1, r2
    mov r2, r3
    /* Load r3, r4, r5 from stack (previous r4, r5, r7 are pushed) */
    ldr r3, [sp, #12]
    ldr r4, [sp, #16]
    ldr r5, [sp, #20]
    svc #0
    pop {r4, r5, r7}
    bx lr
