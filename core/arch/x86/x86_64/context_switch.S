#include "trap_offsets.inc"
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

    movq %rbx, BH_CTX_REG0_OFF(%rdi)
    movq %rbp, BH_CTX_REG1_OFF(%rdi)
    movq %r12, BH_CTX_REG2_OFF(%rdi)
    movq %r13, BH_CTX_REG3_OFF(%rdi)
    movq %r14, BH_CTX_REG4_OFF(%rdi)
    movq %r15, BH_CTX_REG5_OFF(%rdi)

    pushfq
    popq %rax
    movq %rax, BH_CTX_REG6_OFF(%rdi)

    movq BH_CTX_EXT_OFF(%rdi), %rax
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
    movq %rax, BH_CTX_PC_OFF(%rdi)

    leaq 8(%rsp), %rax
    movq %rax, BH_CTX_SP_OFF(%rdi)

.L_restore:
    movq BH_CTX_SP_OFF(%rsi), %rsp

    movq BH_CTX_PC_OFF(%rsi), %rax
    pushq %rax

    movq BH_CTX_REG6_OFF(%rsi), %rax
    pushq %rax
    popfq

    movq BH_CTX_EXT_OFF(%rsi), %rax
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

    movq BH_CTX_REG0_OFF(%rsi), %rbx
    movq BH_CTX_REG1_OFF(%rsi), %rbp
    movq BH_CTX_REG2_OFF(%rsi), %r12
    movq BH_CTX_REG3_OFF(%rsi), %r13
    movq BH_CTX_REG4_OFF(%rsi), %r14
    movq BH_CTX_REG5_OFF(%rsi), %r15

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



.global x86_64_thread_entry_stub
.type x86_64_thread_entry_stub, @function

x86_64_thread_entry_stub:
    cli
    movq 0(%rsp), %rdi
    movq 8(%rsp), %rax
    addq $16, %rsp
    sti
    jmp *%rax

.size x86_64_thread_entry_stub, .-x86_64_thread_entry_stub
