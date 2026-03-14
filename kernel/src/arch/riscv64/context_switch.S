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

    # Save sstatus (equivalent to rflags for RISC-V supervisor)
    csrr t0, sstatus
    sd t0, 96(a0)

    # Save fs0-fs11 (floating point callee-saved)
    # CPU context extended size must support this
    fsd fs0, 144(a0)
    fsd fs1, 152(a0)
    fsd fs2, 160(a0)
    fsd fs3, 168(a0)
    fsd fs4, 176(a0)
    fsd fs5, 184(a0)
    fsd fs6, 192(a0)
    fsd fs7, 200(a0)
    fsd fs8, 208(a0)
    fsd fs9, 216(a0)
    fsd fs10, 224(a0)
    fsd fs11, 232(a0)

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

    # Restore sstatus
    ld t0, 96(a1)
    csrw sstatus, t0

    # Restore fs0-fs11 (floating point callee-saved)
    fld fs0, 144(a1)
    fld fs1, 152(a1)
    fld fs2, 160(a1)
    fld fs3, 168(a1)
    fld fs4, 176(a1)
    fld fs5, 184(a1)
    fld fs6, 192(a1)
    fld fs7, 200(a1)
    fld fs8, 208(a1)
    fld fs9, 216(a1)
    fld fs10, 224(a1)
    fld fs11, 232(a1)

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

    # Call post switch hook to release runqueue lock
    # We must preserve a0 and a1. Also need to ensure sp is 16-byte aligned.
    # It should be 16-byte aligned already by C convention, but we'll just push them safely.
    addi sp, sp, -16
    sd a0, 0(sp)
    sd a1, 8(sp)
    call arch_post_switch
    ld a1, 8(sp)
    ld a0, 0(sp)
    addi sp, sp, 16

    # Return
    ret
