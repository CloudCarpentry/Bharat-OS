.section .text.boot
.global _start
_start:
    /* Preserve device tree pointer passed in r2 by bootloader/QEMU */
    mov r8, r2

    /* Read CPU ID (MPIDR) */
    mrc p15, 0, r1, c0, c0, 5
    and r1, r1, #0xFF

    /* Only let CPU 0 continue for now */
    cmp r1, #0
    bne .secondary_core

    /* Setup stack for primary core */
    ldr r2, =stack_bottom
    mov r3, #8192 /* 8 KiB stack per core */
    mul r4, r1, r3
    add r2, r2, r4
    add r2, r2, r3 /* Point to top of core's stack */
    mov sp, r2

    /* Clear BSS */
    ldr r2, =bss_start
    ldr r3, =bss_end
    cmp r3, #0
    beq 2f
1:
    cmp r2, r3
    bhs 2f
    mov r4, #0
    str r4, [r2], #4
    b 1b
2:

    /* Jump to C kernel_main */
    mov r0, r8
    bl kernel_main

.secondary_core:
    /* Setup stack for secondary cores too */
    ldr r2, =stack_bottom
    mov r3, #8192 /* 8 KiB stack per core */
    mul r4, r1, r3
    add r2, r2, r4
    add r2, r2, r3 /* Point to top of core's stack */
    mov sp, r2
.halt_loop:
    wfi
    b .halt_loop

.section .bss, "aw", %nobits
.align 8
stack_bottom:
    .space 32768 /* 8 KiB stack * 4 cores */
stack_top:
