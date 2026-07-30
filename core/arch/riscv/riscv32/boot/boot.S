/* boot.S — RISC-V 32-bit kernel entry
 *
 * OpenSBI jumps here in Supervisor mode (S-mode) with:
 *   a0 = hart_id      (uint32_t — only 32 bits on RV32)
 *   a1 = dtb_phys_addr (uint32_t physical address)
 *
 * We park all harts except hart 0.  On QEMU virt the primary hart is
 * always hart 0, so the bnez a0 check is sufficient.
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
    sw zero, 0(t0)
    addi t0, t0, 4
    bltu t0, t1, 1b
2:

    /*
     * Jump to C kernel_main(hart_id, fdt_ptr).
     * a0 = hart_id (== 0 here), a1 = dtb_phys_addr — both 32-bit.
     * The C signature uses uint32_t to match RV32 register width.
     */
    call kernel_main

.halt_loop:
    wfi
    j .halt_loop

.section .bss, "aw", @nobits
.align 16
stack_bottom:
    .skip 16384     /* 16 KiB stack for primary hart */
stack_top:
