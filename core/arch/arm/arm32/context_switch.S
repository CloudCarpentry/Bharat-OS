#include "trap_offsets.inc"
.syntax unified
.section .text
.global arch_context_switch
.type arch_context_switch, %function

/* r0 = prev (cpu_context_t*), r1 = next (cpu_context_t*) */
arch_context_switch:
    cmp r0, #0
    beq .L_restore

    /* Save the native-width state using offsets generated from cpu_context_t. */
    str r0, [r0, #BH_CTX_REG0_OFF]       /* regs[0] = r0 */
    str r1, [r0, #BH_CTX_REG1_OFF]       /* regs[1] = r1 */
    str r4, [r0, #BH_CTX_REG4_OFF]      /* regs[4] = r4 */
    str r5, [r0, #BH_CTX_REG5_OFF]      /* regs[5] = r5 */
    str r6, [r0, #BH_CTX_REG6_OFF]      /* regs[6] = r6 */
    str r7, [r0, #BH_CTX_REG7_OFF]      /* regs[7] = r7 */
    str r8, [r0, #BH_CTX_REG8_OFF]      /* regs[8] = r8 */
    str r9, [r0, #BH_CTX_REG9_OFF]      /* regs[9] = r9 */
    str r10, [r0, #BH_CTX_REG10_OFF]     /* regs[10] = r10 */
    str r11, [r0, #BH_CTX_REG11_OFF]     /* regs[11] = r11 */

    str lr, [r0, #BH_CTX_PC_OFF]     /* pc = lr */
    mov r2, sp
    str r2, [r0, #BH_CTX_SP_OFF]     /* sp */

.L_restore:
    ldr r2, [r1, #BH_CTX_SP_OFF]     /* sp */
    mov sp, r2

    ldr lr, [r1, #BH_CTX_PC_OFF]     /* pc */

    ldr r0, [r1, #BH_CTX_REG0_OFF]
    ldr r4, [r1, #BH_CTX_REG4_OFF]
    ldr r5, [r1, #BH_CTX_REG5_OFF]
    ldr r6, [r1, #BH_CTX_REG6_OFF]
    ldr r7, [r1, #BH_CTX_REG7_OFF]
    ldr r8, [r1, #BH_CTX_REG8_OFF]
    ldr r9, [r1, #BH_CTX_REG9_OFF]
    ldr r10, [r1, #BH_CTX_REG10_OFF]
    ldr r11, [r1, #BH_CTX_REG11_OFF]

    push {r0, r1, lr}
    bl arch_post_switch
    pop {r0, r1, lr}

    bx lr

.global arch_bh_thread_start_trampoline
.type arch_bh_thread_start_trampoline, %function
arch_bh_thread_start_trampoline:
    /* r4 holds the true entry point, r0 holds arg0 */
    blx r4
    b sched_thread_exit_trampoline

