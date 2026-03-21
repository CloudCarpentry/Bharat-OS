.section .text.boot
.global _start
_start:
    // Basic ARM32 boot stub. Cortex-M typically boots from vector table at 0x0
    // so this is a placeholder if we treat it as generic arm32.
    b .
