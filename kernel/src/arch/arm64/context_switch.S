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

    str x30,      [x0, #128]

    mov x9, sp
    str x9,       [x0, #136]

.L_restore:
    ldr x9,       [x1, #136]
    mov sp, x9

    ldr x30,      [x1, #128]

    ldp x19, x20, [x1, #0]
    ldp x21, x22, [x1, #16]
    ldp x23, x24, [x1, #32]
    ldp x25, x26, [x1, #48]
    ldp x27, x28, [x1, #64]
    ldr x29,      [x1, #80]

    ret
