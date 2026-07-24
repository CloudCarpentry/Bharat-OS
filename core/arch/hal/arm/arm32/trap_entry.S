.section .text
.global trap_entry
.global vector_table_arm32

.align 5
vector_table_arm32:
    b reset_handler
    b undef_handler
    b svc_handler
    b prefetch_abort_handler
    b data_abort_handler
    b .             /* reserved */
    b irq_handler
    b fiq_handler

reset_handler:          b .
undef_handler:          b .
prefetch_abort_handler: b .
data_abort_handler:     b .
fiq_handler:            b .

svc_handler:
    /* SVC mode. lr points to the instruction after SVC. sp is the SVC stack (kernel stack). */
    /* Allocate trap_frame_t (160 bytes) on kernel stack */
    sub sp, sp, #160

    /* Save r0-r12 */
    stmia sp, {r0-r12}

    /* Save spsr */
    mrs r0, spsr
    tst r0, #0xF
    bne .L_from_kernel

    /* From User Mode */
    mov r1, #1          /* from_user = 1 */
    /* Save user SP and LR */
    add r2, sp, #52
    stm r2, {sp, lr}^
    b .L_save_metadata

.L_from_kernel:
    /* From Kernel Mode */
    mov r1, #0          /* from_user = 0 */
    add r2, sp, #160
    str r2, [sp, #52]   /* gpr[13] is sp */
    str lr, [sp, #56]   /* gpr[14] is lr */

.L_save_metadata:
    str r1, [sp, #148]  /* frame->from_user */

    /* Save spsr (status) */
    mrs r2, spsr
    str r2, [sp, #140]  /* frame->status */

    /* Save pc (lr is the return address) */
    str lr, [sp, #132]  /* frame->pc */

    /* Save type (SYNC = 0) */
    mov r2, #0
    str r2, [sp, #144]  /* frame->type */

    /* Save cause (SVC immediate or pseudo-vector 8) */
    mov r2, #8
    str r2, [sp, #136]  /* frame->cause */

    /* Call trap_handle(frame) */
    mov r0, sp
    bl trap_handle

    /* Restore spsr and pc */
    ldr r2, [sp, #140]
    msr spsr, r2
    ldr lr, [sp, #132]

    /* Restore r0-r12 */
    ldmia sp, {r0-r12}

    /* Pop frame */
    add sp, sp, #160

    /* Return from exception and restore CPSR from SPSR */
    movs pc, lr

irq_handler:
    b svc_handler

trap_entry:
    b svc_handler
