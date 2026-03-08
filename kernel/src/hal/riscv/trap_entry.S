.section .text
.align 4

.extern trap_handle

.global trap_entry
trap_entry:
    # 1. Atomically swap sp and sscratch
    csrrw sp, sscratch, sp

    # 2. If sp is non-zero, we came from U-mode (sscratch held the kernel stack).
    # We can safely branch and continue.
    bnez sp, .L_from_user

    # 3. If sp IS zero, we trapped from S-mode (sscratch held 0).
    # The swap clobbered our kernel sp with 0.
    # Swap them back immediately to restore the kernel sp.
    csrrw sp, sscratch, sp

.L_from_user:
    # sp now safely points to the kernel stack regardless of origin.
    # Allocate trap_frame_t (size = 31*8 + 8 + 8 + 8 + 8 + 1 = 281 bytes, align to 288)
    addi sp, sp, -288

    # Save gpr[] (x1 to x31). x0 is zero.
    sd x1, 0(sp)
    # x2 (sp) requires special handling. Let's save the *original* sp.
    # The original sp is either what we swapped into sscratch (if from S-mode, it's our current sp + 288, if from U-mode, it's what's currently in sscratch).
    # We'll save a dummy for now and fix it later.
    sd x3, 16(sp)
    sd x4, 24(sp)
    sd x5, 32(sp)
    sd x6, 40(sp)
    sd x7, 48(sp)
    sd x8, 56(sp)
    sd x9, 64(sp)
    sd x10, 72(sp)
    sd x11, 80(sp)
    sd x12, 88(sp)
    sd x13, 96(sp)
    sd x14, 104(sp)
    sd x15, 112(sp)
    sd x16, 120(sp)
    sd x17, 128(sp)
    sd x18, 136(sp)
    sd x19, 144(sp)
    sd x20, 152(sp)
    sd x21, 160(sp)
    sd x22, 168(sp)
    sd x23, 176(sp)
    sd x24, 184(sp)
    sd x25, 192(sp)
    sd x26, 200(sp)
    sd x27, 208(sp)
    sd x28, 216(sp)
    sd x29, 224(sp)
    sd x30, 232(sp)
    sd x31, 240(sp)

    # Now fix up sp (x2)
    # We can read sscratch.
    # If we came from U-mode, sscratch holds the user's sp.
    # If we came from S-mode, sscratch holds 0.
    csrr t0, sscratch
    bnez t0, .L_save_user_sp
    # Came from S-mode: original sp is current sp + 288
    addi t0, sp, 288
.L_save_user_sp:
    sd t0, 8(sp)      # Save to gpr[1] (x2/sp)
    sd t0, 248(sp)    # Save to frame->sp (offset 248)

    # Read CSRs
    csrr t0, sepc
    csrr t1, sstatus
    csrr t2, scause

    sd t0, 256(sp)    # frame->pc
    sd t1, 272(sp)    # frame->status
    sd t2, 264(sp)    # frame->cause

    # Determine type (IRQ if MSB of scause is 1)
    srli t0, t2, 63
    sw t0, 280(sp)    # frame->type (TRAP_TYPE_SYNC=0, TRAP_TYPE_IRQ=1)

    # Determine from_user (SPP bit in sstatus: bit 8)
    andi t0, t1, 0x100
    seqz t0, t0       # If SPP == 0, from_user = 1
    sw t0, 284(sp)    # frame->from_user

    # Pass trap_frame_t pointer to C handler
    mv a0, sp
    call trap_handle

    # Restore state
    # First, get from_user and potentially restore sstatus/sepc
    ld t0, 256(sp)    # frame->pc
    ld t1, 272(sp)    # frame->status

    csrw sepc, t0
    csrw sstatus, t1

    # Check if we are returning to U-mode (SPP == 0)
    andi t0, t1, 0x100
    bnez t0, .L_restore_smode

    # Returning to U-mode: we need to setup sscratch to point to our kernel stack again.
    # Our current kernel stack pointer is (sp + 288).
    addi t0, sp, 288
    csrw sscratch, t0

.L_restore_smode:
    # Restore gpr[]
    ld x1, 0(sp)
    # Skip x2 (sp) for a moment
    ld x3, 16(sp)
    ld x4, 24(sp)
    ld x5, 32(sp)
    ld x6, 40(sp)
    ld x7, 48(sp)
    ld x8, 56(sp)
    ld x9, 64(sp)
    ld x10, 72(sp)
    ld x11, 80(sp)
    ld x12, 88(sp)
    ld x13, 96(sp)
    ld x14, 104(sp)
    ld x15, 112(sp)
    ld x16, 120(sp)
    ld x17, 128(sp)
    ld x18, 136(sp)
    ld x19, 144(sp)
    ld x20, 152(sp)
    ld x21, 160(sp)
    ld x22, 168(sp)
    ld x23, 176(sp)
    ld x24, 184(sp)
    ld x25, 192(sp)
    ld x26, 200(sp)
    ld x27, 208(sp)
    ld x28, 216(sp)
    ld x29, 224(sp)
    ld x30, 232(sp)
    ld x31, 240(sp)

    # Finally, restore sp
    ld sp, 248(sp)

    sret
