.intel_syntax noprefix
.global bharat_syscall

bharat_syscall:
    # sysno: rdi
    # arg1: rsi
    # arg2: rdx
    # arg3: rcx -> r10 (for syscall instruction)
    # arg4: r8
    # arg5: r9
    # arg6: [rsp+8] -> stack (we'll just use 5 args for simplicity if we can,
    # but the task says 6 args)

    # Register Mapping for Bharat x86_64:
    # rax: syscall number
    # rdi, rsi, rdx, r10, r8, r9: args 1-6

    mov rax, rdi
    mov rdi, rsi
    mov rsi, rdx
    mov rdx, rcx
    mov r10, r8
    mov r8, r9
    mov r9, [rsp + 8]

    syscall
    ret
