.section .text
.global arc32_boot_stub
.global arc32_trap_entry

arc32_boot_stub:
    /* Disable interrupts */
    clri

    /* Set up stack pointer */
    mov sp, _stack_top

    /* Clear BSS */
    // ... BSS clear logic ...

    /* Jump to kernel entry */
    jl kernel_main

arc32_trap_entry:
    /* Save context */
    // push registers...

    /* Call C handler */
    // jl arch_trap_handler

    /* Restore context */
    // pop registers...
    rtie
