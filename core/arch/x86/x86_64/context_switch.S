.code64
.section .text

.global arch_context_switch
.type arch_context_switch, @function

.extern g_x86_ext_state_use_xsave
.extern g_x86_ext_state_xcr0_lo
.extern g_x86_ext_state_xcr0_hi

# void arch_context_switch(cpu_context_t* prev, cpu_context_t* next);
# rdi = prev (pointer to cpu_context_t), rsi = next (pointer to cpu_context_t)

arch_context_switch:
    test %rdi, %rdi
    jz .L_restore

    movq %rbx, 0(%rdi)
    movq %rbp, 8(%rdi)
    movq %r12, 16(%rdi)
    movq %r13, 24(%rdi)
    movq %r14, 32(%rdi)
    movq %r15, 40(%rdi)

    pushfq
    popq %rax
    movq %rax, 48(%rdi)

    movq 400(%rdi), %rax
    test %rax, %rax
    jz .L_skip_save_fp

    movl g_x86_ext_state_use_xsave(%rip), %ecx
    test %ecx, %ecx
    jz .L_do_fxsave

    movq %rax, %r8
    movl g_x86_ext_state_xcr0_lo(%rip), %eax
    movl g_x86_ext_state_xcr0_hi(%rip), %edx
    xsave (%r8)
    jmp .L_skip_save_fp

.L_do_fxsave:
    fxsave (%rax)
.L_skip_save_fp:

    movq (%rsp), %rax
    movq %rax, 128(%rdi)

    leaq 8(%rsp), %rax
    movq %rax, 136(%rdi)

.L_restore:
    movq 136(%rsi), %rsp

    movq 128(%rsi), %rax
    pushq %rax

    movq 48(%rsi), %rax
    pushq %rax
    popfq

    movq 400(%rsi), %rax
    test %rax, %rax
    jz .L_skip_restore_fp

    movl g_x86_ext_state_use_xsave(%rip), %ecx
    test %ecx, %ecx
    jz .L_do_fxrstor

    movq %rax, %r8
    movl g_x86_ext_state_xcr0_lo(%rip), %eax
    movl g_x86_ext_state_xcr0_hi(%rip), %edx
    xrstor (%r8)
    jmp .L_skip_restore_fp

.L_do_fxrstor:
    fxrstor (%rax)
.L_skip_restore_fp:

    movq 0(%rsi), %rbx
    movq 8(%rsi), %rbp
    movq 16(%rsi), %r12
    movq 24(%rsi), %r13
    movq 32(%rsi), %r14
    movq 40(%rsi), %r15

    pushq %rsi
    pushq %rbx
    movq %rsp, %rbx
    andq $-16, %rsp
    call arch_post_switch
    movq %rbx, %rsp
    popq %rbx
    popq %rsi

    popq %rax
    jmp *%rax
