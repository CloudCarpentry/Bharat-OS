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
    sub sp, sp, #288
    stp x0, x1, [sp, #0]
    mov w0, #\type
    str w0, [sp, #280] // frame->type
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
    /* Original x0, x1 are at [sp, #0] */
    /* Store all other GPRs */
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
    str x30, [sp, #240]

    /* Save system state registers */
    mrs x21, spsr_el1
    str x21, [sp, #272] // frame->status

    mrs x20, elr_el1
    str x20, [sp, #256] // frame->pc

    mrs x20, esr_el1
    str x20, [sp, #264] // frame->cause

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
    add x20, sp, #288
    mov w23, #0
2:
    str x20, [sp, #248] // frame->sp
    str w23, [sp, #284] // frame->from_user (matches trap.h layout)

    /* If this is a syscall (SVC), esr_el1[31:26] == 0b010101 (0x15).
     * The SVC instruction is always 4 bytes, so we increment ELR by 4 to return to the next instruction.
     */
    ldr x20, [sp, #264] // frame->cause (esr_el1)
    lsr x20, x20, #26
    cmp x20, #0x15
    b.ne 10f

    ldr x20, [sp, #256] // frame->pc
    add x20, x20, #4
    str x20, [sp, #256] // write back advanced pc
10:

    /* Call C Handler */
    mov x0, sp
    bl trap_handle

    /* Restore Architecture state */
    ldr x20, [sp, #256] 
    msr elr_el1, x20

    ldr x21, [sp, #272]
    msr spsr_el1, x21

    /* Restore User Stack if returning to EL0 */
    and x22, x21, #0xF
    cmp x22, #0
    b.ne 3f
    ldr x20, [sp, #248]
    msr sp_el0, x20
3:

    /* Restore GPRs */
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
