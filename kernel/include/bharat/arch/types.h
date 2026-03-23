#ifndef BHARAT_ARCH_TYPES_H
#define BHARAT_ARCH_TYPES_H

/*
 * Compile-time architecture word size and address space contracts.
 */

#if defined(__x86_64__) || defined(__aarch64__) || (defined(__riscv) && __riscv_xlen == 64) || defined(__LP64__)
    #define ARCH_WORD_BITS  64
    #define ARCH_PADDR_BITS 64 /* Can be refined by specific arch configs, but use 64 as safe container */
    #define ARCH_VADDR_BITS 64
#elif defined(__arm__) || (defined(__riscv) && __riscv_xlen == 32) || defined(__i386__)
    #define ARCH_WORD_BITS  32
    #define ARCH_PADDR_BITS 32 /* Some ARM systems have LPAE (40-bit), but default to 32 */
    #define ARCH_VADDR_BITS 32
#else
    #error "Unknown architecture word size"
#endif

#endif /* BHARAT_ARCH_TYPES_H */
