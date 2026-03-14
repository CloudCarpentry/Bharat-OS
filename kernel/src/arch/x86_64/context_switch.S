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

    # Save rflags
    pushfq
    popq %rax
    movq %rax, 48(%rdi)

    # Save FPU state if enabled (pointer at offset 400)
    # The pointer is prev->ext (arch_ext_state_t *). If not NULL, save to it.
    movq 400(%rdi), %rax
    test %rax, %rax
    jz .L_skip_fxsave
    # Save FPU state using fxsave
    # It requires 16-byte alignment, kmem_aligned_alloc gave us 64-byte alignment
    fxsave (%rax)
.L_skip_fxsave:

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

    # Restore rflags
    movq 48(%rsi), %rax
    pushq %rax
    popfq

    # Restore FPU state if enabled
    movq 400(%rsi), %rax
    test %rax, %rax
    jz .L_skip_fxrstor
    fxrstor (%rax)
.L_skip_fxrstor:

    # Restore callee-saved registers
    movq 0(%rsi), %rbx
    movq 8(%rsi), %rbp
    movq 16(%rsi), %r12
    movq 24(%rsi), %r13
    movq 32(%rsi), %r14
    movq 40(%rsi), %r15

    # Release runqueue lock if necessary here (we'll implement this via a call hook)
    pushq %rdi
    pushq %rsi
    # Align stack before C function call
    # SysV ABI: %rsp must be 16-byte aligned *before* the call instruction.
    movq %rsp, %rax
    andq $0xFFFFFFFFFFFFFFF0, %rsp
    # The stack is now 16-byte aligned. But if we push %rax, it becomes 8-byte aligned again.
    # We must push %rax (8 bytes) and then subtract 8 bytes to maintain 16-byte alignment before 'call'.
    pushq %rax
    subq $8, %rsp
    call arch_post_switch
    addq $8, %rsp
    popq %rax
    movq %rax, %rsp
    popq %rsi
    popq %rdi

    # Pop the return address into rax to maintain stack alignment, then jmp to it.
    popq %rax
    jmp *%rax
