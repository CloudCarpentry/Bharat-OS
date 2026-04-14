/* boot.S — ARM64 kernel entry
 * QEMU -kernel boots directly here in EL1 or EL2.
 *
 * x0 = physical address of device tree blob (dtb)
 */

.section .text.init
.global _start
_start:
    /* ARM64 Linux Kernel Image Header (Must be exactly 64 bytes) */
    b       .real_start         /* 0x00: code0 */
    .word   0                   /* 0x04: code1 (padding) */
    .quad   0x80000             /* 0x08: text_offset */
    .quad   0x0                 /* 0x10: image_size */
    .quad   0x0                 /* 0x18: flags */
    .quad   0x0                 /* 0x20: res2 */
    .quad   0x0                 /* 0x28: res3 */
    .quad   0x0                 /* 0x30: res4 */
    .ascii  "ARM\x64"            /* 0x38: magic */
    .word   0x0                 /* 0x3c: res5 */

.real_start:
    /* Preserve device tree pointer passed in x0 by bootloader/QEMU */
    mov x19, x0

    /* Read CPU ID (MPIDR_EL1) and only let CPU 0 continue for now */
    mrs x1, mpidr_el1
    and x1, x1, #0xFF
    cbnz x1, .halt_loop

    /* Setup stack */
    ldr x1, =stack_top
    mov sp, x1

    /* Clear BSS */
    ldr x1, =bss_start
    ldr x2, =bss_end
    cbz x2, 2f
1:
    cmp x1, x2
    b.hs 2f
    str xzr, [x1], #8
    b 1b
2:

    /* Enable FPU/SIMD at EL1 */
    mrs x1, cpacr_el1
    orr x1, x1, #(3 << 20) /* FPEN bits 20:21 = 0b11 */
    msr cpacr_el1, x1
    isb

    /* Jump to C kernel_main */
    mov x0, x19
    bl kernel_main

.halt_loop:
    wfi
    b .halt_loop

.section .bss, "aw", @nobits
.align 16
stack_bottom:
    .skip 16384 /* 16 KiB stack */
stack_top:
