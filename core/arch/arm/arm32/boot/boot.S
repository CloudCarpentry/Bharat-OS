.section .text.boot
.global _start
_start:
    /* Preserve device tree pointer passed in r2 by bootloader/QEMU */
    mov r8, r2

    /*
     * Early PL011 UART init (QEMU virt: base 0x09000000)
     * Enables TX+RX and the UART block (UARTCR = 0x301).
     * Must happen before any C code that calls hal_serial_write so that
     * the "BOOT: kernel_main reached" marker appears on the console.
     */
    ldr r0, =0x09000000
    mov r1, #0x301          /* TXE | RXE | UARTEN */
    str r1, [r0, #0x30]     /* UARTCR */

    /* Read CPU ID (MPIDR) */
    mrc p15, 0, r1, c0, c0, 5
    and r1, r1, #0xFF

    /* Only let CPU 0 continue for now */
    cmp r1, #0
    bne .secondary_core

    /* Setup stack for primary core (CPU 0: r1 == 0) */
    ldr r2, =stack_bottom
    mov r3, #8192           /* 8 KiB stack per core */
    mul r4, r1, r3          /* offset = cpu_id * stack_size = 0 for CPU 0 */
    add r2, r2, r4
    add r2, r2, r3          /* sp = stack_bottom + offset + stack_size */
    mov sp, r2

    /* Clear BSS: loop from bss_start to bss_end writing 0 */
    ldr r2, =bss_start
    ldr r3, =bss_end
    cmp r2, r3              /* skip if bss_start >= bss_end (empty BSS) */
    bhs 2f
1:
    mov r4, #0
    str r4, [r2], #4
    cmp r2, r3
    blo 1b
2:

    /* Jump to C kernel_main(fdt_ptr=r8) */
    mov r0, r8
    bl kernel_main

.secondary_core:
    /* Setup stack for secondary cores */
    ldr r2, =stack_bottom
    mov r3, #8192           /* 8 KiB stack per core */
    mul r4, r1, r3
    add r2, r2, r4
    add r2, r2, r3
    mov sp, r2
.halt_loop:
    wfi
    b .halt_loop

.section .bss, "aw", %nobits
.align 8
stack_bottom:
    .space 32768            /* 8 KiB stack × 4 cores */
stack_top:
