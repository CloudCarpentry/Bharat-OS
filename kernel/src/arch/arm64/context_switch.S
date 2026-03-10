.text
.global arch_context_switch
.type arch_context_switch, @function

# void arch_context_switch(cpu_context_t* prev, cpu_context_t* next);
# x0 = prev (pointer to cpu_context_t), x1 = next (pointer to cpu_context_t)

arch_context_switch:
    # If prev is NULL, skip saving context
    cbz x0, .L_restore

    # Save callee-saved registers x19-x29
    # cpu_context_t layout: uint64_t regs[16]; uint64_t pc; uint64_t sp;
    stp x19, x20, [x0, #0]    # regs[0], regs[1]
    stp x21, x22, [x0, #16]   # regs[2], regs[3]
    stp x23, x24, [x0, #32]   # regs[4], regs[5]
    stp x25, x26, [x0, #48]   # regs[6], regs[7]
    stp x27, x28, [x0, #64]   # regs[8], regs[9]
    str x29,      [x0, #80]   # regs[10] (frame pointer)

    # x30 is the link register (return address)
    str x30,      [x0, #128]  # pc

    # Save the current stack pointer
    mov x9, sp
    str x9,       [x0, #136]  # sp

.L_restore:
    # Restore stack pointer
    ldr x9,       [x1, #136]  # sp
    mov sp, x9

    # Restore return address into x30
    ldr x30,      [x1, #128]  # pc

    # Restore callee-saved registers
    ldp x19, x20, [x1, #0]
    ldp x21, x22, [x1, #16]
    ldp x23, x24, [x1, #32]
    ldp x25, x26, [x1, #48]
    ldp x27, x28, [x1, #64]
    ldr x29,      [x1, #80]

    # Return
    ret
