.text
.global arch_context_switch
.type arch_context_switch, @function

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

    // Save d8-d15 (floating point callee-saved)
    stp d8, d9,   [x0, #144]
    stp d10, d11, [x0, #160]
    stp d12, d13, [x0, #176]
    stp d14, d15, [x0, #192]

    str x30,      [x0, #128]

    mov x9, sp
    str x9,       [x0, #136]

.L_restore:
    ldr x9,       [x1, #136]
    mov sp, x9

    ldr x30,      [x1, #128]

    ldr x9,       [x1, #96]
    msr daif, x9

    // Restore d8-d15 (floating point callee-saved)
    ldp d8, d9,   [x1, #144]
    ldp d10, d11, [x1, #160]
    ldp d12, d13, [x1, #176]
    ldp d14, d15, [x1, #192]

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
