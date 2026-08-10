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
    /* Preserve device tree pointer passed in x0 by bootloader/QEMU */
    mov x19, x0

    /* Read CPU ID (MPIDR_EL1) */
    mrs x1, mpidr_el1
    and x1, x1, #0xFF

    /* Only let CPU 0 continue for now */
    cbnz x1, .secondary_core_park

    /* Setup stack for primary core */
    ldr x2, =stack_bottom
    mov x3, #16384 /* 16 KiB stack per core */
    mul x4, x1, x3
    add x2, x2, x4
    add x2, x2, x3 /* Point to top of core's stack */
    mov sp, x2

    /* Clear BSS */
    ldr x2, =bss_start
    ldr x3, =bss_end
    cbz x3, 2f
1:
    cmp x2, x3
    b.hs 2f
    str xzr, [x2], #8
    b 1b
2:

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

    /* Running at EL2: allow all FP/SIMD and CPACR accesses from EL1 (clear CPTR_EL2) */
    msr cptr_el2, xzr
    isb

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
    /* Setup stack for EL1h */
    ldr x2, =stack_bottom
    mov x3, #16384
    add x2, x2, x3
    mov sp, x2

    /* Now definitely at EL1 - enable FPU/SIMD */
    mrs x2, cpacr_el1
    orr x2, x2, #(3 << 20)    /* FPEN = 0b11: no FP/SIMD traps */
    msr cpacr_el1, x2
    isb

    /* Jump to C kernel_main */
    mov x0, x19
    bl kernel_main

    /* Should not return */
    b .halt_loop

.secondary_core_park:
    /* Setup stack for secondary cores too if they ever start here */
    ldr x2, =stack_bottom
    mov x3, #16384 /* 16 KiB stack per core */
    mul x4, x1, x3
    add x2, x2, x4
    add x2, x2, x3 /* Point to top of core's stack */
    mov sp, x2
.halt_loop:
    wfi
    b .halt_loop

.global _secondary_trampoline
_secondary_trampoline:
    /* 1. Mask exceptions */
    msr daifset, #0xf

    /* 2. Check EL2 and drop to EL1 if needed */
    mrs x2, CurrentEL
    lsr x2, x2, #2
    cmp x2, #2
    b.ne .ap_at_el1

    /* Running at EL2: clear CPTR_EL2 traps and drop to EL1 */
    msr cptr_el2, xzr
    isb
    mov x2, #(1 << 31)
    msr hcr_el2, x2
    msr sctlr_el1, xzr
    isb
    mov x2, #0x3c5             /* DAIF=1111, M[3:0]=0101 -> EL1h */
    msr spsr_el2, x2
    adr x2, .ap_at_el1
    msr elr_el2, x2
    eret

.ap_at_el1:
    /* Now definitely at EL1 - enable FPU/SIMD */
    mrs x2, cpacr_el1
    orr x2, x2, #(3 << 20)    /* FPEN = 0b11: no FP/SIMD traps */
    msr cpacr_el1, x2
    isb

    /* 3. Validate context pointer in x0 */
    cbz x0, .halt_loop

    /* Save context pointer in x19 */
    mov x19, x0

    /* 4. Select the per-core physical stack from the context */
    /* context layout:
     *   logical_cpu_id: offset 0
     *   stack_top_phys: offset 8
     *   cpu_local_phys: offset 16
     *   kernel_root_phys: offset 24
     *   ttbr0_root_phys: offset 32
     *   tcr_el1: offset 40
     *   mair_el1: offset 48
     *   sctlr_el1: offset 56
     *   vbar_el1: offset 64
     */
    ldr x1, [x19, #8]         /* stack_top_phys */
    mov sp, x1

    /* 5. Install MAIR_EL1 */
    ldr x2, [x19, #48]        /* mair_el1 */
    msr mair_el1, x2

    /* 6. Install TCR_EL1 */
    ldr x3, [x19, #40]        /* tcr_el1 */
    msr tcr_el1, x3

    /* 7. Install the kernel translation root (TTBR1_EL1) */
    ldr x4, [x19, #24]        /* kernel_root_phys */
    msr ttbr1_el1, x4

    /* 8. Install safe identity translation root to TTBR0_EL1 so low-half PC doesn't fault */
    msr ttbr0_el1, x4

    /* 9. Barriers */
    dsb sy
    isb

    /* 10. Enable MMU and caches through SCTLR_EL1 */
    ldr x6, [x19, #56]        /* sctlr_el1 */
    msr sctlr_el1, x6
    isb

    /* 11. Switch SP and references to virtual addresses if we were using high-half.
     * But since we are booted on identity map, we can continue or jump to virtual addresses directly.
     * Let's load the virtual address of the C entry point.
     */
    ldr x7, [x19, #64]        /* vbar_el1 */
    msr vbar_el1, x7
    isb

    /* Call bh_secondary_cpu_entry(context_phys) */
    mov x0, x19
    ldr x8, =bh_secondary_cpu_entry
    br x8

.section .bss, "aw", @nobits
.align 16
stack_bottom:
    .skip 65536 /* 16 KiB stack * 4 cores */
stack_top:
