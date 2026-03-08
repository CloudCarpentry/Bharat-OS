.section .text
.align 11
.global vector_table_el1

// Macro for a vector table entry (must be 0x80 bytes apart)
.macro VECTOR_ENTRY label
.align 7
    b \label
.endm

vector_table_el1:
    // Current EL with SP0 (Synchronous, IRQ, FIQ, SError)
    VECTOR_ENTRY trap_common
    VECTOR_ENTRY trap_common
    VECTOR_ENTRY trap_common
    VECTOR_ENTRY trap_common

    // Current EL with SPx
    VECTOR_ENTRY trap_common
    VECTOR_ENTRY trap_common
    VECTOR_ENTRY trap_common
    VECTOR_ENTRY trap_common

    // Lower EL using AArch64
    VECTOR_ENTRY trap_common
    VECTOR_ENTRY trap_common
    VECTOR_ENTRY trap_common
    VECTOR_ENTRY trap_common

    // Lower EL using AArch32
    VECTOR_ENTRY trap_common
    VECTOR_ENTRY trap_common
    VECTOR_ENTRY trap_common
    VECTOR_ENTRY trap_common

.global trap_common
trap_common:
    // We assume we are in EL1 and have switched to SP_EL1.
    // Allocate 288 bytes on stack to form trap_frame_t
    sub sp, sp, #288

    // Save GPRs (x0-x30). x30 is LR.
    // In ARM64, trap_frame_t has uint64_t gpr[31].
    stp x0, x1, [sp, #0]
    stp x2, x3, [sp, #16]
    stp x4, x5, [sp, #32]
    stp x6, x7, [sp, #48]
    stp x8, x9, [sp, #64]
    stp x10, x11, [sp, #80]
    stp x12, x13, [sp, #96]
    stp x14, x15, [sp, #112]
    stp x16, x17, [sp, #128]
    stp x18, x19, [sp, #144]
    stp x20, x21, [sp, #160]
    stp x22, x23, [sp, #176]
    stp x24, x25, [sp, #192]
    stp x26, x27, [sp, #208]
    stp x28, x29, [sp, #224]
    str x30, [sp, #240] // gpr[30]
    // gpr[31] is unused in ARM64 typically, or mapped to SP. We'll leave it as 0 or ignore it.

    // Calculate original SP (from before trap)
    // If from user (EL0), SP is in SP_EL0.
    // If from kernel (EL1), SP is what SP was before we did "sub sp, sp, #288"
    mrs x21, spsr_el1
    and x22, x21, #0xF // M[3:0]
    cmp x22, #0        // Is it EL0t (M=0b0000)?
    b.ne 1f
    // From EL0
    mrs x20, sp_el0
    b 2f
1:
    // From EL1
    add x20, sp, #288
2:
    str x20, [sp, #248] // frame->sp

    // Read PC (ELR_EL1), Status (SPSR_EL1), Cause (ESR_EL1)
    mrs x20, elr_el1
    str x20, [sp, #256] // frame->pc

    mrs x20, esr_el1
    str x20, [sp, #264] // frame->cause

    // SPSR_EL1 is already in x21
    str x21, [sp, #272] // frame->status

    // Set from_user flag
    // If M[3:0] == 0, then from_user = 1, else 0.
    // x22 has M[3:0].
    cmp x22, #0
    cset w20, eq
    strb w20, [sp, #280] // frame->from_user

    // Call C handler
    mov x0, sp // Pass pointer to trap_frame_t in x0
    bl trap_handle

    // Restore state
    ldr x20, [sp, #256] // frame->pc
    msr elr_el1, x20

    ldr x21, [sp, #272] // frame->status
    msr spsr_el1, x21

    // Check if returning to user mode to restore SP_EL0
    and x22, x21, #0xF
    cmp x22, #0
    b.ne 3f
    ldr x20, [sp, #248] // frame->sp
    msr sp_el0, x20
3:

    // Restore GPRs
    ldp x0, x1, [sp, #0]
    ldp x2, x3, [sp, #16]
    ldp x4, x5, [sp, #32]
    ldp x6, x7, [sp, #48]
    ldp x8, x9, [sp, #64]
    ldp x10, x11, [sp, #80]
    ldp x12, x13, [sp, #96]
    ldp x14, x15, [sp, #112]
    ldp x16, x17, [sp, #128]
    ldp x18, x19, [sp, #144]
    ldp x20, x21, [sp, #160]
    ldp x22, x23, [sp, #176]
    ldp x24, x25, [sp, #192]
    ldp x26, x27, [sp, #208]
    ldp x28, x29, [sp, #224]
    ldr x30, [sp, #240]

    add sp, sp, #288
    eret
