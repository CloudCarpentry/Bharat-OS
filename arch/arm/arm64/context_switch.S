.text
.global arch_context_switch
.type arch_context_switch, @function

// x0 = prev cpu_context_t*
// x1 = next cpu_context_t*
arch_context_switch:
    cbz x0, .L_restore

    stp x19, x20, [x0, #0]
    stp x21, x22, [x0, #16]
    stp x23, x24, [x0, #32]
    stp x25, x26, [x0, #48]
    stp x27, x28, [x0, #64]
    str x29,      [x0, #80]

    // Save DAIF (interrupt masks / flags)
    mrs x9, daif
    str x9,       [x0, #96]

    str x30,      [x0, #128]

    mov x9, sp
    str x9,       [x0, #136]

    // Lazy ext-state save hook
    stp x0, x1, [sp, #-16]!
    bl arch_ext_state_context_switch_out
    ldp x0, x1, [sp], #16

.L_restore:
    // Lazy ext-state restore hook
    stp x0, x1, [sp, #-16]!
    mov x0, x1
    bl arch_ext_state_context_switch_in
    ldp x0, x1, [sp], #16

    ldr x9,       [x1, #136]
    mov sp, x9

    ldr x30,      [x1, #128]

    ldr x9,       [x1, #96]
    msr daif, x9

    ldp x19, x20, [x1, #0]
    ldp x21, x22, [x1, #16]
    ldp x23, x24, [x1, #32]
    ldp x25, x26, [x1, #48]
    ldp x27, x28, [x1, #64]
    ldr x29,      [x1, #80]

    // Call arch_post_switch to release runqueue lock
    // It is called after switching the stack and basic state,
    // but before executing user code or returning to the thread's execution path.
    // Preserve x0, x1 as they might be used by the caller
    stp x0, x1, [sp, #-16]!
    stp x29, x30, [sp, #-16]!
    mov x29, sp
    bl arch_post_switch
    ldp x29, x30, [sp], #16
    ldp x0, x1, [sp], #16

    ret

.global arch_bh_thread_start_trampoline
.type arch_bh_thread_start_trampoline, @function
arch_bh_thread_start_trampoline:
    // x19 holds the true entry point, setup by arch_prepare_initial_context
    mov x0, x19
    blr x0
    // After returning from the entry point, jump to the exit trampoline
    b sched_thread_exit_trampoline

.global arm64_fp_state_save
.type arm64_fp_state_save, @function
// x0 = arch_ext_state_t*
// offset 16 is where qregs start
arm64_fp_state_save:
    add x1, x0, #16
    stp q0, q1, [x1, #0]
    stp q2, q3, [x1, #32]
    stp q4, q5, [x1, #64]
    stp q6, q7, [x1, #96]
    stp q8, q9, [x1, #128]
    stp q10, q11, [x1, #160]
    stp q12, q13, [x1, #192]
    stp q14, q15, [x1, #224]
    stp q16, q17, [x1, #256]
    stp q18, q19, [x1, #288]
    stp q20, q21, [x1, #320]
    stp q22, q23, [x1, #352]
    stp q24, q25, [x1, #384]
    stp q26, q27, [x1, #416]
    stp q28, q29, [x1, #448]
    stp q30, q31, [x1, #480]

    mrs x2, fpsr
    str w2, [x0, #528]
    mrs x3, fpcr
    str w3, [x0, #532]

    ret

.global arm64_fp_state_restore
.type arm64_fp_state_restore, @function
// x0 = arch_ext_state_t*
// offset 16 is where qregs start
arm64_fp_state_restore:
    add x1, x0, #16
    ldp q0, q1, [x1, #0]
    ldp q2, q3, [x1, #32]
    ldp q4, q5, [x1, #64]
    ldp q6, q7, [x1, #96]
    ldp q8, q9, [x1, #128]
    ldp q10, q11, [x1, #160]
    ldp q12, q13, [x1, #192]
    ldp q14, q15, [x1, #224]
    ldp q16, q17, [x1, #256]
    ldp q18, q19, [x1, #288]
    ldp q20, q21, [x1, #320]
    ldp q22, q23, [x1, #352]
    ldp q24, q25, [x1, #384]
    ldp q26, q27, [x1, #416]
    ldp q28, q29, [x1, #448]
    ldp q30, q31, [x1, #480]

    ldr w2, [x0, #528]
    msr fpsr, x2
    ldr w3, [x0, #532]
    msr fpcr, x3

    ret
