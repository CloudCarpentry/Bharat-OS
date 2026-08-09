#include "trap_offsets.inc"

.section .text
.global trap_entry
.global vector_table_arm32

/* ARM exception classes have different LR adjustments and must never share an
 * SVC classification path.  Each vector supplies its semantic class/cause to
 * the common, bounded frame builder. */
.macro ARM32_VECTOR name, type, cause, lr_adjust
\name:
    sub sp, sp, #BH_TF_STACK_SIZE
    stmia sp, {r0-r12}

    mrs r3, spsr
    and r2, r3, #0x1f
    cmp r2, #0x10                 /* CPSR user mode */
    moveq r1, #1
    movne r1, #0
    str r1, [sp, #BH_TF_FROM_USER_OFF]

    /* User SP/LR are banked and can only be copied with the ^ form. */
    cmp r1, #0
    beq 1f
    add r2, sp, #BH_TF_GPR13_OFF
    stm r2, {sp, lr}^
    ldr r2, [sp, #BH_TF_GPR13_OFF]
    b 2f
1:
    add r2, sp, #BH_TF_STACK_SIZE
    str r2, [sp, #BH_TF_GPR13_OFF]
    str lr, [sp, #BH_TF_GPR14_OFF]
2:
    str r2, [sp, #BH_TF_SP_OFF]
    str r3, [sp, #BH_TF_STATUS_OFF]

.if \lr_adjust
    sub r2, lr, #\lr_adjust
.else
    mov r2, lr
.endif
    str r2, [sp, #BH_TF_PC_OFF]
    mov r2, #\type
    str r2, [sp, #BH_TF_TYPE_OFF]
    mov r2, #\cause
    str r2, [sp, #BH_TF_CAUSE_OFF]

    mov r0, sp
    bl trap_handle

    ldr r2, [sp, #BH_TF_STATUS_OFF]
    msr spsr_cxsf, r2
    ldr lr, [sp, #BH_TF_PC_OFF]
    ldmia sp, {r0-r12}
    add sp, sp, #BH_TF_STACK_SIZE
    movs pc, lr
.endm

.align 5
vector_table_arm32:
    b reset_handler
    b undef_handler
    b svc_handler
    b prefetch_abort_handler
    b data_abort_handler
    b reserved_handler
    b irq_handler
    b fiq_handler

reset_handler: b .
reserved_handler: b .

/* cause uses the architectural vector offset, not an SVC pseudo-cause. */
ARM32_VECTOR undef_handler,          0,  4, 4
ARM32_VECTOR svc_handler,            0,  8, 0
ARM32_VECTOR prefetch_abort_handler, 0, 12, 4
ARM32_VECTOR data_abort_handler,     0, 16, 8
ARM32_VECTOR irq_handler,            1, 24, 4
ARM32_VECTOR fiq_handler,            2, 28, 4

trap_entry:
    b svc_handler
