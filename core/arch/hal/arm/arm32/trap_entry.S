.section .text
.global trap_entry
trap_entry:
    /* arm32 trap path is a stub */
1:
    wfi
    b 1b
