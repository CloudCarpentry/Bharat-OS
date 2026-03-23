.text
.global arch_context_switch
.type arch_context_switch, @function

# void arch_context_switch(cpu_context_t* prev, cpu_context_t* next);
# a0 = prev (pointer to cpu_context_t), a1 = next (pointer to cpu_context_t)

arch_context_switch:
    beqz a0, .L_restore

    sd s0,  0(a0)
    sd s1,  8(a0)
    sd s2,  16(a0)
    sd s3,  24(a0)
    sd s4,  32(a0)
    sd s5,  40(a0)
    sd s6,  48(a0)
    sd s7,  56(a0)
    sd s8,  64(a0)
    sd s9,  72(a0)
    sd s10, 80(a0)
    sd s11, 88(a0)

    csrr t0, sstatus
    sd t0, 96(a0)

    sd ra, 128(a0)
    sd sp, 136(a0)

    addi sp, sp, -16
    sd a0, 0(sp)
    sd a1, 8(sp)
    call arch_ext_state_context_switch_out
    ld a1, 8(sp)
    ld a0, 0(sp)
    addi sp, sp, 16

.L_restore:
    addi sp, sp, -16
    sd a0, 0(sp)
    sd a1, 8(sp)
    mv a0, a1
    call arch_ext_state_context_switch_in
    ld a1, 8(sp)
    ld a0, 0(sp)
    addi sp, sp, 16

    ld sp, 136(a1)
    ld ra, 128(a1)

    ld t0, 96(a1)
    csrw sstatus, t0

    ld s0,  0(a1)
    ld s1,  8(a1)
    ld s2,  16(a1)
    ld s3,  24(a1)
    ld s4,  32(a1)
    ld s5,  40(a1)
    ld s6,  48(a1)
    ld s7,  56(a1)
    ld s8,  64(a1)
    ld s9,  72(a1)
    ld s10, 80(a1)
    ld s11, 88(a1)

    addi sp, sp, -32
    sd a0, 0(sp)
    sd a1, 8(sp)
    sd ra, 16(sp)
    call arch_post_switch
    ld ra, 16(sp)
    ld a1, 8(sp)
    ld a0, 0(sp)
    addi sp, sp, 32

    ret

.global riscv_fp_state_save
.type riscv_fp_state_save, @function
// a0 = arch_ext_state_t*
// offset 16 is where fpr starts
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
// a0 = arch_ext_state_t*
// offset 16 is where fpr starts
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
