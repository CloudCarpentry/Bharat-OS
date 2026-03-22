.text
.global arch_context_switch
.type arch_context_switch, @function

# void arch_context_switch(cpu_context_t* prev, cpu_context_t* next);
# a0 = prev (pointer to cpu_context_t), a1 = next (pointer to cpu_context_t)

arch_context_switch:
    # If prev is NULL, skip saving context
    beqz a0, .L_restore

    # Save callee-saved registers into prev->regs
    # cpu_context_t layout: uint64_t regs[16]; uint64_t pc; uint64_t sp;
    sd s0,  0(a0)    # regs[0] (s0/fp)
    sd s1,  8(a0)    # regs[1]
    sd s2,  16(a0)   # regs[2]
    sd s3,  24(a0)   # regs[3]
    sd s4,  32(a0)   # regs[4]
    sd s5,  40(a0)   # regs[5]
    sd s6,  48(a0)   # regs[6]
    sd s7,  56(a0)   # regs[7]
    sd s8,  64(a0)   # regs[8]
    sd s9,  72(a0)   # regs[9]
    sd s10, 80(a0)   # regs[10]
    sd s11, 88(a0)   # regs[11]

    # Save ra (return address) into pc
    sd ra, 128(a0)   # pc

    # Save stack pointer
    sd sp, 136(a0)   # sp

.L_restore:
    # Restore from next context

    # Restore stack pointer
    ld sp, 136(a1)   # sp

    # Restore return address
    ld ra, 128(a1)   # pc

    # Restore callee-saved registers
    ld s0,  0(a1)    # regs[0]
    ld s1,  8(a1)    # regs[1]
    ld s2,  16(a1)   # regs[2]
    ld s3,  24(a1)   # regs[3]
    ld s4,  32(a1)   # regs[4]
    ld s5,  40(a1)   # regs[5]
    ld s6,  48(a1)   # regs[6]
    ld s7,  56(a1)   # regs[7]
    ld s8,  64(a1)   # regs[8]
    ld s9,  72(a1)   # regs[9]
    ld s10, 80(a1)   # regs[10]
    ld s11, 88(a1)   # regs[11]

    # Return
    ret
