#ifndef BHARAT_ARCH_HASH_H
#define BHARAT_ARCH_HASH_H

#include <stddef.h>
#include <stdint.h>

/**
 * @file hash.h
 * @brief Architecture-specific hardware-accelerated hash operations.
 *
 * Provides architecture hooks to leverage hardware CRC32/CRC64 instructions
 * (e.g., x86 `CRC32`, ARM `CRC32`, RISC-V `Zbb`) for fast O(1) hash table lookups.
 */

/**
 * @brief Architecture-optimized hash function.
 *
 * @param key The 64-bit key to hash.
 * @param seed A seed or modifier (e.g., table index) to produce different hash variations.
 * @param size The size of the hash table.
 * @return The computed hash index (modulo size).
 */
size_t arch_hash_func(uint64_t key, int seed, size_t size);

#endif /* BHARAT_ARCH_HASH_H */
