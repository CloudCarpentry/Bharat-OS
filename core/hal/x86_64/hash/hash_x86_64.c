/*
 * hal/x86_64/hash/hash_x86_64.c
 *
 * x86-64 hardware-accelerated CRC32 hash function.
 */

#include "arch/hash.h"
#include <immintrin.h>

size_t arch_hash_func(uint64_t key, int seed, size_t size) {
    uint64_t hash;
    if (seed == 0) {
        hash = _mm_crc32_u64(0, key);
    } else {
        hash = _mm_crc32_u64(0xdeadbeef, key);
    }
    return hash % size;
}
