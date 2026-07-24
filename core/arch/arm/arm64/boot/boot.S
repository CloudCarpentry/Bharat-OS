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

.macro UART_INIT
    mov x12, #0x09000000
    mov w13, #0x101
    str w13, [x12, #0x30]
.endm

.macro UART_PUT_CHAR_LIT char
    mov x12, #0x09000000
    mov w13, #\char
    str w13, [x12]
.endm

.macro UART_PUT_REG reg
    mov x12, #0x09000000
    str \reg, [x12]
.endm

.real_start:
    UART_INIT
    UART_PUT_CHAR_LIT '1'
    /* Preserve device tree pointer passed in x0 by bootloader/QEMU */
    mov x19, x0
    UART_PUT_CHAR_LIT '2'

    /* Read CPU ID (MPIDR_EL1) */
    mrs x1, mpidr_el1
    and x1, x1, #0xFF
    UART_PUT_CHAR_LIT '3'

    /* Only let CPU 0 continue for now */
    cbnz x1, .secondary_core
    UART_PUT_CHAR_LIT '4'

    /* Setup stack for primary core */
    ldr x2, =stack_bottom
    mov x3, #16384 /* 16 KiB stack per core */
    mul x4, x1, x3
    add x2, x2, x4
    add x2, x2, x3 /* Point to top of core's stack */
    mov sp, x2
    UART_PUT_CHAR_LIT '5'

    /* Clear BSS */
    ldr x2, =bss_start
    ldr x3, =bss_end
    UART_PUT_CHAR_LIT '6'
    cbz x3, 2f
1:
    cmp x2, x3
    b.hs 2f
    str xzr, [x2], #8
    b 1b
2:
    UART_PUT_CHAR_LIT '7'

    /* -------------------------------------------------------
     * EL2 -> EL1 transition
     * QEMU virt starts primary CPU at EL2.  By default
     * CPTR_EL2.TFP (bit 10) = 1, which traps all FP/SIMD
     * instructions to EL2.  Since we have no EL2 handler the
     * CPU crashes silently on the first SIMD instruction.
     * We must either stay at EL2 and clear TFP, or drop to EL1.
     * We drop to EL1 (matches Linux boot protocol).
     * ------------------------------------------------------- */
    mrs x2, CurrentEL
    lsr x2, x2, #2
    cmp x2, #2
    b.ne .at_el1

    /* Running at EL2: allow FP/SIMD from EL1/EL0 (clear CPTR_EL2.TFP) */
    mrs x2, cptr_el2
    bic x2, x2, #(1 << 10)
    msr cptr_el2, x2

    /* HCR_EL2.RW = 1 so EL1 runs AArch64 */
    mov x2, #(1 << 31)
    msr hcr_el2, x2

    /* Safe SCTLR_EL1 reset value */
    msr sctlr_el1, xzr
    isb

    /* Return to EL1h with DAIF masked */
    mov x2, #0x3c5             /* DAIF=1111, M[3:0]=0101 -> EL1h */
    msr spsr_el2, x2
    adr x2, .at_el1
    msr elr_el2, x2
    eret

.at_el1:
    /* Now definitely at EL1 - enable FPU/SIMD */
    mrs x2, cpacr_el1
    orr x2, x2, #(3 << 20)    /* FPEN = 0b11: no FP/SIMD traps */
    msr cpacr_el1, x2
    isb
    UART_PUT_CHAR_LIT '8'

    /* Jump to C kernel_main */
    mov x0, x19
    bl kernel_main

    /* Should not return */
    b .halt_loop

.secondary_core:
    /* Setup stack for secondary cores too */
    ldr x2, =stack_bottom
    mov x3, #16384 /* 16 KiB stack per core */
    mul x4, x1, x3
    add x2, x2, x4
    add x2, x2, x3 /* Point to top of core's stack */
    mov sp, x2
.halt_loop:
    wfi
    b .halt_loop

.section .bss, "aw", @nobits
.align 16
stack_bottom:
    .skip 65536 /* 16 KiB stack * 4 cores */
stack_top:
