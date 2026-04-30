.section .text
.global trap_entry
trap_entry:
    /* riscv32 trap path is a stub */
1:
    wfi
    j 1b
