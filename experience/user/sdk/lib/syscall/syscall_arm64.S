.global bharat_syscall

bharat_syscall:
    /*
     * x8: syscall number (from x0)
     * x0..x5: args (from x1..x6)
     */
    mov x8, x0
    mov x0, x1
    mov x1, x2
    mov x2, x3
    mov x3, x4
    mov x4, x5
    mov x5, x6
    svc #0
    ret
