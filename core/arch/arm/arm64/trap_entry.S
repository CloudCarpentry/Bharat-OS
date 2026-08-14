#include "trap_offsets.inc"
.section .text
.align 11
.global vector_table_el1

/*
 * VECTOR_ENTRY_TYPE macro:
 * 1. Allocates trap_frame_t on stack (288 bytes)
 * 2. Saves x0, x1 immediately
 * 3. Overwrites w0 with the trap type
 * 4. Branches to trap_common_logic
 */
.macro VECTOR_ENTRY_TYPE type
.align 7
    sub sp, sp, #BH_ARM64_TF_STACK_SIZE
    stp x0, x1, [sp, #BH_TF_GPR0_OFF]
    mov w0, #\type
    str w0, [sp, #BH_TF_TYPE_OFF] // frame->type
    b trap_common_logic
.endm

vector_table_el1:
    /* Current EL with SP0 */
    VECTOR_ENTRY_TYPE 0 // Sync
    VECTOR_ENTRY_TYPE 1 // IRQ
    VECTOR_ENTRY_TYPE 2 // FIQ
    VECTOR_ENTRY_TYPE 3 // SError

    /* Current EL with SPx */
    VECTOR_ENTRY_TYPE 0
    VECTOR_ENTRY_TYPE 1
    VECTOR_ENTRY_TYPE 2
    VECTOR_ENTRY_TYPE 3

    /* Lower EL using AArch64 */
    VECTOR_ENTRY_TYPE 0
    VECTOR_ENTRY_TYPE 1
    VECTOR_ENTRY_TYPE 2
    VECTOR_ENTRY_TYPE 3

    /* Lower EL using AArch32 */
    VECTOR_ENTRY_TYPE 0
    VECTOR_ENTRY_TYPE 1
    VECTOR_ENTRY_TYPE 2
    VECTOR_ENTRY_TYPE 3

.global trap_common_logic
trap_common_logic:
    /* Original x0, x1 are at [sp, #BH_TF_GPR0_OFF] */
    /* Store all other GPRs */
    stp x2, x3, [sp, #BH_TF_GPR2_OFF]
    stp x4, x5, [sp, #BH_TF_GPR4_OFF]
    stp x6, x7, [sp, #BH_TF_GPR6_OFF]
    stp x8, x9, [sp, #BH_TF_GPR8_OFF]
    stp x10, x11, [sp, #BH_TF_GPR10_OFF]
    stp x12, x13, [sp, #BH_TF_GPR12_OFF]
    stp x14, x15, [sp, #BH_TF_GPR14_OFF]
    stp x16, x17, [sp, #BH_TF_GPR16_OFF]
    stp x18, x19, [sp, #BH_TF_GPR18_OFF]
    stp x20, x21, [sp, #BH_TF_GPR20_OFF]
    stp x22, x23, [sp, #BH_TF_GPR22_OFF]
    stp x24, x25, [sp, #BH_TF_GPR24_OFF]
    stp x26, x27, [sp, #BH_TF_GPR26_OFF]
    stp x28, x29, [sp, #BH_TF_GPR28_OFF]
    str x30, [sp, #BH_TF_GPR30_OFF]

    /* Save system state registers */
    mrs x21, spsr_el1
    str x21, [sp, #BH_TF_STATUS_OFF] // frame->status

    mrs x20, elr_el1
    str x20, [sp, #BH_TF_PC_OFF] // frame->pc

    mrs x20, esr_el1
    str x20, [sp, #BH_TF_CAUSE_OFF] // frame->cause

    mrs x20, far_el1
    str x20, [sp, #BH_ARM64_TF_FAULT_ADDR_OFF]

    /* Handle stack pointers and from_user flag */
    and x22, x21, #0xF // M[3:0]
    cmp x22, #0        // EL0t?
    b.ne 1f

    /* From User Mode (EL0) */
    mrs x20, sp_el0
    mov w23, #1
    b 2f
1:
    /* From Kernel Mode (EL1) */
    add x20, sp, #BH_ARM64_TF_STACK_SIZE
    mov w23, #0
2:
    str x20, [sp, #BH_TF_SP_OFF] // frame->sp
    str w23, [sp, #BH_TF_FROM_USER_OFF] // frame->from_user (matches trap.h layout)

    /* If this is a syscall (SVC), esr_el1[31:26] == 0b010101 (0x15).
     * The SVC instruction is always 4 bytes, so we increment ELR by 4 to return to the next instruction.
     */
    ldr x20, [sp, #BH_TF_CAUSE_OFF] // frame->cause (esr_el1)
    lsr x20, x20, #26
    cmp x20, #0x15
    b.ne 10f

    ldr x20, [sp, #BH_TF_PC_OFF] // frame->pc
    add x20, x20, #4
    str x20, [sp, #BH_TF_PC_OFF] // write back advanced pc
10:

    /* Call C Handler */
    mov x0, sp
    bl trap_handle

    /* Restore Architecture state */
    ldr x20, [sp, #BH_TF_PC_OFF]
    msr elr_el1, x20

    ldr x21, [sp, #BH_TF_STATUS_OFF]
    msr spsr_el1, x21

    /* Restore User Stack if returning to EL0 */
    and x22, x21, #0xF
    cmp x22, #0
    b.ne 3f
    ldr x20, [sp, #BH_TF_SP_OFF]
    msr sp_el0, x20
3:

    /* Restore GPRs */
    ldp x0, x1, [sp, #BH_TF_GPR0_OFF]
    ldp x2, x3, [sp, #BH_TF_GPR2_OFF]
    ldp x4, x5, [sp, #BH_TF_GPR4_OFF]
    ldp x6, x7, [sp, #BH_TF_GPR6_OFF]
    ldp x8, x9, [sp, #BH_TF_GPR8_OFF]
    ldp x10, x11, [sp, #BH_TF_GPR10_OFF]
    ldp x12, x13, [sp, #BH_TF_GPR12_OFF]
    ldp x14, x15, [sp, #BH_TF_GPR14_OFF]
    ldp x16, x17, [sp, #BH_TF_GPR16_OFF]
    ldp x18, x19, [sp, #BH_TF_GPR18_OFF]
    ldp x20, x21, [sp, #BH_TF_GPR20_OFF]
    ldp x22, x23, [sp, #BH_TF_GPR22_OFF]
    ldp x24, x25, [sp, #BH_TF_GPR24_OFF]
    ldp x26, x27, [sp, #BH_TF_GPR26_OFF]
    ldp x28, x29, [sp, #BH_TF_GPR28_OFF]
    ldr x30, [sp, #BH_TF_GPR30_OFF]

    add sp, sp, #BH_ARM64_TF_STACK_SIZE
    eret
