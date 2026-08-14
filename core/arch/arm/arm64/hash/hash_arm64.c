/*
 * hal/arm64/hash/hash_arm64.c
 *
 * ARM64 hardware-accelerated CRC32 hash function.
 */

#include "arch/hash.h"
#include <arm_acle.h>

size_t arch_hash_func(uint64_t key, int seed, size_t size) {
    uint32_t hash;
    if (seed == 0) {
        hash = __crc32d(0, key);
    } else {
        hash = __crc32d(0xdeadbeef, key);
    }
    return hash % size;
}
