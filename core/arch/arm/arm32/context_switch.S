.syntax unified
.section .text
.global arch_context_switch
.type arch_context_switch, %function

/* r0 = prev (cpu_context_t*), r1 = next (cpu_context_t*) */
arch_context_switch:
    cmp r0, #0
    beq .L_restore

    /* Save prev state into cpu_context_t (each entry is 64-bit/8-byte aligned) */
    str r0, [r0, #0]       /* regs[0] = r0 */
    str r1, [r0, #8]       /* regs[1] = r1 */
    str r4, [r0, #32]      /* regs[4] = r4 */
    str r5, [r0, #40]      /* regs[5] = r5 */
    str r6, [r0, #48]      /* regs[6] = r6 */
    str r7, [r0, #56]      /* regs[7] = r7 */
    str r8, [r0, #64]      /* regs[8] = r8 */
    str r9, [r0, #72]      /* regs[9] = r9 */
    str r10, [r0, #80]     /* regs[10] = r10 */
    str r11, [r0, #88]     /* regs[11] = r11 */

    str lr, [r0, #128]     /* pc = lr */
    mov r2, sp
    str r2, [r0, #136]     /* sp */

.L_restore:
    ldr r2, [r1, #136]     /* sp */
    mov sp, r2

    ldr lr, [r1, #128]     /* pc */

    ldr r0, [r1, #0]
    ldr r4, [r1, #32]
    ldr r5, [r1, #40]
    ldr r6, [r1, #48]
    ldr r7, [r1, #56]
    ldr r8, [r1, #64]
    ldr r9, [r1, #72]
    ldr r10, [r1, #80]
    ldr r11, [r1, #88]

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

