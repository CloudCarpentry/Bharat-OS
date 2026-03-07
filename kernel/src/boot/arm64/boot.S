/* boot.S — ARM64 kernel entry
 * QEMU -kernel boots directly here in EL1 or EL2.
 *
 * x0 = physical address of device tree blob (dtb)
 */

.section .text.init
.global _start
_start:
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

    /* Jump to C kernel_main */
    bl kernel_main

.halt_loop:
    wfi
    b .halt_loop

.section .bss, "aw", @nobits
.align 16
stack_bottom:
    .skip 16384 /* 16 KiB stack */
stack_top:
