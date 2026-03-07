/* boot.S — RISC-V 64-bit kernel entry
 * OpenSBI jumps here in Supervisor mode (S-mode) with:
 *   a0 = hartid
 *   a1 = dtb_phys_addr
 */

.section .text
.global kernel_main

.section .text.init
.global _start
_start:
    /* Only allow hart 0 to boot the kernel initially.
     * OpenSBI passes hartid in a0.
     */
    bnez a0, .halt_loop

    /* Setup stack */
    la sp, stack_top

    /* Clear BSS */
    la t0, bss_start
    la t1, bss_end
    beq t0, t1, 2f
1:
    sd zero, (t0)
    addi t0, t0, 8
    bltu t0, t1, 1b
2:

    /* Jump to C kernel_main */
    call kernel_main

.halt_loop:
    wfi
    j .halt_loop

.section .bss, "aw", @nobits
.align 16
stack_bottom:
    .skip 16384 /* 16 KiB stack */
stack_top:
