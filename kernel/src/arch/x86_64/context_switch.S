.code64
.section .text

.global arch_context_switch
.type arch_context_switch, @function

# void arch_context_switch(cpu_context_t* prev, cpu_context_t* next);
# rdi = prev (pointer to cpu_context_t), rsi = next (pointer to cpu_context_t)

arch_context_switch:
    # If prev is NULL, skip saving context
    test %rdi, %rdi
    jz .L_restore

    # Save callee-saved registers into prev->regs
    # cpu_context_t layout: uint64_t regs[16]; uint64_t pc; uint64_t sp;
    movq %rbx, 0(%rdi)
    movq %rbp, 8(%rdi)
    movq %r12, 16(%rdi)
    movq %r13, 24(%rdi)
    movq %r14, 32(%rdi)
    movq %r15, 40(%rdi)

    # Save return address (which is currently on top of the stack)
    movq (%rsp), %rax
    movq %rax, 128(%rdi) # prev->pc

    # Save stack pointer (as it was before the call, so %rsp + 8)
    leaq 8(%rsp), %rax
    movq %rax, 136(%rdi) # prev->sp

.L_restore:
    # Restore from next context

    # Restore stack pointer
    movq 136(%rsi), %rsp # next->sp

    # Push the restored return address onto the stack, so `ret` will jump to it
    movq 128(%rsi), %rax # next->pc
    pushq %rax

    # Restore callee-saved registers
    movq 0(%rsi), %rbx
    movq 8(%rsi), %rbp
    movq 16(%rsi), %r12
    movq 24(%rsi), %r13
    movq 32(%rsi), %r14
    movq 40(%rsi), %r15

    # Return to next context
    ret
