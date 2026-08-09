#include "trap_offsets.inc"
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
    # The target C compiler supplies the aligned raw-frame size.
    addi sp, sp, -BH_RISCV_TF_STACK_SIZE

    # Save gpr[] (x1 to x31). x0 is zero.
    SREG x1, BH_TF_GPR0_OFF(sp)
    # x2 (sp) saved later
    SREG x3, BH_TF_GPR2_OFF(sp)
    SREG x4, BH_TF_GPR3_OFF(sp)
    SREG x5, BH_TF_GPR4_OFF(sp)
    SREG x6, BH_TF_GPR5_OFF(sp)
    SREG x7, BH_TF_GPR6_OFF(sp)
    SREG x8, BH_TF_GPR7_OFF(sp)
    SREG x9, BH_TF_GPR8_OFF(sp)
    SREG x10, BH_TF_GPR9_OFF(sp)
    SREG x11, BH_TF_GPR10_OFF(sp)
    SREG x12, BH_TF_GPR11_OFF(sp)
    SREG x13, BH_TF_GPR12_OFF(sp)
    SREG x14, BH_TF_GPR13_OFF(sp)
    SREG x15, BH_TF_GPR14_OFF(sp)
    SREG x16, BH_TF_GPR15_OFF(sp)
    SREG x17, BH_TF_GPR16_OFF(sp)
    SREG x18, BH_TF_GPR17_OFF(sp)
    SREG x19, BH_TF_GPR18_OFF(sp)
    SREG x20, BH_TF_GPR19_OFF(sp)
    SREG x21, BH_TF_GPR20_OFF(sp)
    SREG x22, BH_TF_GPR21_OFF(sp)
    SREG x23, BH_TF_GPR22_OFF(sp)
    SREG x24, BH_TF_GPR23_OFF(sp)
    SREG x25, BH_TF_GPR24_OFF(sp)
    SREG x26, BH_TF_GPR25_OFF(sp)
    SREG x27, BH_TF_GPR26_OFF(sp)
    SREG x28, BH_TF_GPR27_OFF(sp)
    SREG x29, BH_TF_GPR28_OFF(sp)
    SREG x30, BH_TF_GPR29_OFF(sp)
    SREG x31, BH_TF_GPR30_OFF(sp)

    # Now fix up sp (x2)
    csrr t0, sscratch
    bnez t0, .L_save_user_sp
    # Came from S-mode
    addi t0, sp, BH_RISCV_TF_STACK_SIZE
.L_save_user_sp:
    SREG t0, BH_TF_GPR1_OFF(sp)      # Save to gpr[1] (x2/sp)
    SREG t0, BH_TF_SP_OFF(sp)    # Save to frame->sp (offset 31)

    # Read CSRs
    csrr t0, sepc
    csrr t1, sstatus
    csrr t2, scause

    SREG t0, BH_TF_PC_OFF(sp)    # frame->pc
    SREG t1, BH_TF_STATUS_OFF(sp)    # frame->status (note: check struct offset)
    SREG t2, BH_TF_CAUSE_OFF(sp)    # frame->cause
    csrr t0, stval
    SREG t0, BH_RISCV_TF_FAULT_ADDR_OFF(sp)

    # Determine type (IRQ if MSB of scause is 1)
    SRLI t0, t2, SHIFT_AMOUNT
    sw t0, BH_TF_TYPE_OFF(sp)    # frame->type

    # Determine from_user (SPP bit in sstatus: bit 8)
    andi t0, t1, 0x100
    seqz t0, t0
    sw t0, BH_TF_FROM_USER_OFF(sp)    # frame->from_user

    # Handle syscall PC advance
    LREG t2, BH_TF_CAUSE_OFF(sp)    # frame->cause
    li t3, 8
    bne t2, t3, .L_skip_pc_adj
    LREG t0, BH_TF_PC_OFF(sp)
    addi t0, t0, 4
    SREG t0, BH_TF_PC_OFF(sp)
.L_skip_pc_adj:

    # Pass frame pointer to C
    mv a0, sp
    call trap_handle

    # Restore state
    LREG t0, BH_TF_PC_OFF(sp)    # pc
    LREG t1, BH_TF_STATUS_OFF(sp)    # status
    csrw sepc, t0
    csrw sstatus, t1

    # Check return destination
    andi t0, t1, 0x100
    bnez t0, .L_restore_smode
    addi t0, sp, BH_RISCV_TF_STACK_SIZE
    csrw sscratch, t0

.L_restore_smode:
    LREG x1, BH_TF_GPR0_OFF(sp)
    LREG x3, BH_TF_GPR2_OFF(sp)
    LREG x4, BH_TF_GPR3_OFF(sp)
    LREG x5, BH_TF_GPR4_OFF(sp)
    LREG x6, BH_TF_GPR5_OFF(sp)
    LREG x7, BH_TF_GPR6_OFF(sp)
    LREG x8, BH_TF_GPR7_OFF(sp)
    LREG x9, BH_TF_GPR8_OFF(sp)
    LREG x10, BH_TF_GPR9_OFF(sp)
    LREG x11, BH_TF_GPR10_OFF(sp)
    LREG x12, BH_TF_GPR11_OFF(sp)
    LREG x13, BH_TF_GPR12_OFF(sp)
    LREG x14, BH_TF_GPR13_OFF(sp)
    LREG x15, BH_TF_GPR14_OFF(sp)
    LREG x16, BH_TF_GPR15_OFF(sp)
    LREG x17, BH_TF_GPR16_OFF(sp)
    LREG x18, BH_TF_GPR17_OFF(sp)
    LREG x19, BH_TF_GPR18_OFF(sp)
    LREG x20, BH_TF_GPR19_OFF(sp)
    LREG x21, BH_TF_GPR20_OFF(sp)
    LREG x22, BH_TF_GPR21_OFF(sp)
    LREG x23, BH_TF_GPR22_OFF(sp)
    LREG x24, BH_TF_GPR23_OFF(sp)
    LREG x25, BH_TF_GPR24_OFF(sp)
    LREG x26, BH_TF_GPR25_OFF(sp)
    LREG x27, BH_TF_GPR26_OFF(sp)
    LREG x28, BH_TF_GPR27_OFF(sp)
    LREG x29, BH_TF_GPR28_OFF(sp)
    LREG x30, BH_TF_GPR29_OFF(sp)
    LREG x31, BH_TF_GPR30_OFF(sp)

    LREG sp, BH_TF_SP_OFF(sp)
    sret
