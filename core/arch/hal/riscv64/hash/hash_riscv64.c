/*
 * hal/riscv64/hash/hash_riscv64.c
 *
 * RISC-V 64-bit hardware-accelerated hash function.
 * Falls back to the generic multiplication algorithm until
 * Zbb extension or specific instructions are properly mapped.
 */

#include "arch/hash.h"

/*
 * In a real environment, you might detect Zbb extension and use:
 *   asm volatile ("crc32.d %0, %1" : "=r" (hash) : "r" (key));
 * For now, we fallback to multiplicative hash since inline asm for Zbb
 * isn't universally supported without specific compiler flags (-march=rv64gc_zbb).
 */
size_t arch_hash_func(uint64_t key, int seed, size_t size) {
    if (seed == 0) {
        return (key * 2654435761U) % size;
    } else {
        return ((key * 2654435761U) ^ 0xdeadbeef) % size;
    }
}
