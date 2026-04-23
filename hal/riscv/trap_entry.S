#if __riscv_xlen == 64
#define LREG ld
#define SREG sd
#define REGBYTES 8
#define SRLI srli
#define SHIFT_AMOUNT 63
#else
#define LREG lw
#define SREG sw
#define REGBYTES 4
#define SRLI srli
#define SHIFT_AMOUNT 31
#endif

.section .text
.align 4

.extern trap_handle

.global trap_entry
trap_entry:
    # 1. Atomically swap sp and sscratch
    csrrw sp, sscratch, sp

    # 2. If sp is non-zero, we came from U-mode (sscratch held the kernel stack).
    bnez sp, .L_from_user

    # 3. If sp IS zero, we trapped from S-mode
    csrrw sp, sscratch, sp

.L_from_user:
    # sp now safely points to the kernel stack regardless of origin.
    # Allocate trap_frame_t 
    # Size depends on register size.
    # x1-x31 (31 regs) + sp(1) + pc(1) + status(1) + cause(1) + type(1) + from_user(1) = 37 slots
    # 37 * REGBYTES. For 64-bit: 37*8 = 296. Align to 32 bytes = 320.
    # For 32-bit: 37*4 = 148. Align to 32 bytes = 160.
#if __riscv_xlen == 64
    addi sp, sp, -320
#else
    addi sp, sp, -160
#endif

    # Save gpr[] (x1 to x31). x0 is zero.
    SREG x1, 0*REGBYTES(sp)
    # x2 (sp) saved later
    SREG x3, 2*REGBYTES(sp)
    SREG x4, 3*REGBYTES(sp)
    SREG x5, 4*REGBYTES(sp)
    SREG x6, 5*REGBYTES(sp)
    SREG x7, 6*REGBYTES(sp)
    SREG x8, 7*REGBYTES(sp)
    SREG x9, 8*REGBYTES(sp)
    SREG x10, 9*REGBYTES(sp)
    SREG x11, 10*REGBYTES(sp)
    SREG x12, 11*REGBYTES(sp)
    SREG x13, 12*REGBYTES(sp)
    SREG x14, 13*REGBYTES(sp)
    SREG x15, 14*REGBYTES(sp)
    SREG x16, 15*REGBYTES(sp)
    SREG x17, 16*REGBYTES(sp)
    SREG x18, 17*REGBYTES(sp)
    SREG x19, 18*REGBYTES(sp)
    SREG x20, 19*REGBYTES(sp)
    SREG x21, 20*REGBYTES(sp)
    SREG x22, 21*REGBYTES(sp)
    SREG x23, 22*REGBYTES(sp)
    SREG x24, 23*REGBYTES(sp)
    SREG x25, 24*REGBYTES(sp)
    SREG x26, 25*REGBYTES(sp)
    SREG x27, 26*REGBYTES(sp)
    SREG x28, 27*REGBYTES(sp)
    SREG x29, 28*REGBYTES(sp)
    SREG x30, 29*REGBYTES(sp)
    SREG x31, 30*REGBYTES(sp)

    # Now fix up sp (x2)
    csrr t0, sscratch
    bnez t0, .L_save_user_sp
    # Came from S-mode
#if __riscv_xlen == 64
    addi t0, sp, 320
#else
    addi t0, sp, 160
#endif
.L_save_user_sp:
    SREG t0, 1*REGBYTES(sp)      # Save to gpr[1] (x2/sp)
    SREG t0, 31*REGBYTES(sp)    # Save to frame->sp (offset 31)

    # Read CSRs
    csrr t0, sepc
    csrr t1, sstatus
    csrr t2, scause

    SREG t0, 32*REGBYTES(sp)    # frame->pc
    SREG t1, 34*REGBYTES(sp)    # frame->status (note: check struct offset)
    SREG t2, 33*REGBYTES(sp)    # frame->cause

    # Determine type (IRQ if MSB of scause is 1)
    SRLI t0, t2, SHIFT_AMOUNT
    SREG t0, 35*REGBYTES(sp)    # frame->type 

    # Determine from_user (SPP bit in sstatus: bit 8)
    andi t0, t1, 0x100
    seqz t0, t0       
    SREG t0, 36*REGBYTES(sp)    # frame->from_user

    # Handle syscall PC advance
    LREG t2, 33*REGBYTES(sp)    # frame->cause
    li t3, 8
    bne t2, t3, .L_skip_pc_adj
    LREG t0, 32*REGBYTES(sp)
    addi t0, t0, 4
    SREG t0, 32*REGBYTES(sp)
.L_skip_pc_adj:

    # Pass frame pointer to C
    mv a0, sp
    call trap_handle

    # Restore state
    LREG t0, 32*REGBYTES(sp)    # pc
    LREG t1, 34*REGBYTES(sp)    # status
    csrw sepc, t0
    csrw sstatus, t1

    # Check return destination
    andi t0, t1, 0x100
    bnez t0, .L_restore_smode
#if __riscv_xlen == 64
    addi t0, sp, 320
#else
    addi t0, sp, 160
#endif
    csrw sscratch, t0

.L_restore_smode:
    LREG x1, 0*REGBYTES(sp)
    LREG x3, 2*REGBYTES(sp)
    LREG x4, 3*REGBYTES(sp)
    LREG x5, 4*REGBYTES(sp)
    LREG x6, 5*REGBYTES(sp)
    LREG x7, 6*REGBYTES(sp)
    LREG x8, 7*REGBYTES(sp)
    LREG x9, 8*REGBYTES(sp)
    LREG x10, 9*REGBYTES(sp)
    LREG x11, 10*REGBYTES(sp)
    LREG x12, 11*REGBYTES(sp)
    LREG x13, 12*REGBYTES(sp)
    LREG x14, 13*REGBYTES(sp)
    LREG x15, 14*REGBYTES(sp)
    LREG x16, 15*REGBYTES(sp)
    LREG x17, 16*REGBYTES(sp)
    LREG x18, 17*REGBYTES(sp)
    LREG x19, 18*REGBYTES(sp)
    LREG x20, 19*REGBYTES(sp)
    LREG x21, 20*REGBYTES(sp)
    LREG x22, 21*REGBYTES(sp)
    LREG x23, 22*REGBYTES(sp)
    LREG x24, 23*REGBYTES(sp)
    LREG x25, 24*REGBYTES(sp)
    LREG x26, 25*REGBYTES(sp)
    LREG x27, 26*REGBYTES(sp)
    LREG x28, 27*REGBYTES(sp)
    LREG x29, 28*REGBYTES(sp)
    LREG x30, 29*REGBYTES(sp)
    LREG x31, 30*REGBYTES(sp)

    LREG sp, 31*REGBYTES(sp)
    sret
