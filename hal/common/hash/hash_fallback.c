/*
 * hal/common/hash/hash_fallback.c
 *
 * Generic portable C implementation of the hash function.
 * This acts as the default fallback for architectures that
 * do not provide their own hardware-accelerated CRC32 paths.
 */

#include "arch/hash.h"

size_t arch_hash_func(uint64_t key, int seed, size_t size) {
    if (seed == 0) {
        return (key * 2654435761U) % size;
    } else {
        return ((key * 2654435761U) ^ 0xdeadbeef) % size;
    }
}
