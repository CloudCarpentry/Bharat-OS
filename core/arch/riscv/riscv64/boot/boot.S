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
    /*
     * Park all secondary harts.
     * OpenSBI sets a0=0 for the primary hart before jumping to the payload.
     * Any hart with a0 != 0 is a secondary — spin it in WFI.
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

    /*
     * Jump to C kernel_main(hart_id, fdt_ptr).
     * a0 = hart_id (== 0 here), a1 = dtb_phys_addr — both preserved from
     * OpenSBI handoff and passed directly as the first two arguments.
     */
    call kernel_main

.halt_loop:
    wfi
    j .halt_loop

.global _secondary_trampoline
_secondary_trampoline:
    /* SBI HSM enters with the opaque boot context in a0. */
    tail bh_secondary_cpu_entry

.section .bss, "aw", @nobits
.align 16
stack_bottom:
    .skip 16384 /* 16 KiB stack */
stack_top:
