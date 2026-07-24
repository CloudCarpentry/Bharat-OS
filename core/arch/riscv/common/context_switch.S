#if __riscv_xlen == 64
#define LREG ld
#define SREG sd
#define REGBYTES 8
#else
#define LREG lw
#define SREG sw
#define REGBYTES 4
#endif

.text
.global arch_context_switch
.type arch_context_switch, @function

# void arch_context_switch(cpu_context_t* prev, cpu_context_t* next);
# a0 = prev (pointer to cpu_context_t), a1 = next (pointer to cpu_context_t)

arch_context_switch:
    beqz a0, .L_restore

    SREG s0,  0*REGBYTES(a0)
    SREG s1,  1*REGBYTES(a0)
    SREG s2,  2*REGBYTES(a0)
    SREG s3,  3*REGBYTES(a0)
    SREG s4,  4*REGBYTES(a0)
    SREG s5,  5*REGBYTES(a0)
    SREG s6,  6*REGBYTES(a0)
    SREG s7,  7*REGBYTES(a0)
    SREG s8,  8*REGBYTES(a0)
    SREG s9,  9*REGBYTES(a0)
    SREG s10, 10*REGBYTES(a0)
    SREG s11, 11*REGBYTES(a0)

    csrr t0, sstatus
    SREG t0, 12*REGBYTES(a0)

    SREG ra, 16*REGBYTES(a0)
    SREG sp, 17*REGBYTES(a0)

    addi sp, sp, -(2*REGBYTES)
    SREG a0, 0*REGBYTES(sp)
    SREG a1, 1*REGBYTES(sp)
    call arch_ext_state_context_switch_out
    LREG a1, 1*REGBYTES(sp)
    LREG a0, 0*REGBYTES(sp)
    addi sp, sp, (2*REGBYTES)

.L_restore:
    addi sp, sp, -(2*REGBYTES)
    SREG a0, 0*REGBYTES(sp)
    SREG a1, 1*REGBYTES(sp)
    mv a0, a1
    call arch_ext_state_context_switch_in
    LREG a1, 1*REGBYTES(sp)
    LREG a0, 0*REGBYTES(sp)
    addi sp, sp, (2*REGBYTES)

    LREG sp, 17*REGBYTES(a1)
    LREG ra, 16*REGBYTES(a1)

    LREG t0, 12*REGBYTES(a1)
    csrw sstatus, t0

    LREG s0,  0*REGBYTES(a1)
    LREG s1,  1*REGBYTES(a1)
    LREG s2,  2*REGBYTES(a1)
    LREG s3,  3*REGBYTES(a1)
    LREG s4,  4*REGBYTES(a1)
    LREG s5,  5*REGBYTES(a1)
    LREG s6,  6*REGBYTES(a1)
    LREG s7,  7*REGBYTES(a1)
    LREG s8,  8*REGBYTES(a1)
    LREG s9,  9*REGBYTES(a1)
    LREG s10, 10*REGBYTES(a1)
    LREG s11, 11*REGBYTES(a1)

    addi sp, sp, -(4*REGBYTES)
    SREG a0, 0*REGBYTES(sp)
    SREG a1, 1*REGBYTES(sp)
    SREG ra, 2*REGBYTES(sp)
    call arch_post_switch
    LREG ra, 2*REGBYTES(sp)
    LREG a1, 1*REGBYTES(sp)
    LREG a0, 0*REGBYTES(sp)
    addi sp, sp, (4*REGBYTES)

    ret

.global riscv_fp_state_save
.type riscv_fp_state_save, @function
# a0 = arch_ext_state_t*
riscv_fp_state_save:
    fsd f0,  16(a0)
    fsd f1,  24(a0)
    fsd f2,  32(a0)
    fsd f3,  40(a0)
    fsd f4,  48(a0)
    fsd f5,  56(a0)
    fsd f6,  64(a0)
    fsd f7,  72(a0)
    fsd f8,  80(a0)
    fsd f9,  88(a0)
    fsd f10, 96(a0)
    fsd f11, 104(a0)
    fsd f12, 112(a0)
    fsd f13, 120(a0)
    fsd f14, 128(a0)
    fsd f15, 136(a0)
    fsd f16, 144(a0)
    fsd f17, 152(a0)
    fsd f18, 160(a0)
    fsd f19, 168(a0)
    fsd f20, 176(a0)
    fsd f21, 184(a0)
    fsd f22, 192(a0)
    fsd f23, 200(a0)
    fsd f24, 208(a0)
    fsd f25, 216(a0)
    fsd f26, 224(a0)
    fsd f27, 232(a0)
    fsd f28, 240(a0)
    fsd f29, 248(a0)
    fsd f30, 256(a0)
    fsd f31, 264(a0)

    csrr t0, fcsr
    sw t0, 272(a0)
    ret

.global riscv_fp_state_restore
.type riscv_fp_state_restore, @function
# a0 = arch_ext_state_t*
riscv_fp_state_restore:
    fld f0,  16(a0)
    fld f1,  24(a0)
    fld f2,  32(a0)
    fld f3,  40(a0)
    fld f4,  48(a0)
    fld f5,  56(a0)
    fld f6,  64(a0)
    fld f7,  72(a0)
    fld f8,  80(a0)
    fld f9,  88(a0)
    fld f10, 96(a0)
    fld f11, 104(a0)
    fld f12, 112(a0)
    fld f13, 120(a0)
    fld f14, 128(a0)
    fld f15, 136(a0)
    fld f16, 144(a0)
    fld f17, 152(a0)
    fld f18, 160(a0)
    fld f19, 168(a0)
    fld f20, 176(a0)
    fld f21, 184(a0)
    fld f22, 192(a0)
    fld f23, 200(a0)
    fld f24, 208(a0)
    fld f25, 216(a0)
    fld f26, 224(a0)
    fld f27, 232(a0)
    fld f28, 240(a0)
    fld f29, 248(a0)
    fld f30, 256(a0)
    fld f31, 264(a0)

    lw t0, 272(a0)
    csrw fcsr, t0
    ret
