#include "arch/hash.h"
#include "arch/arch_cpu_caps.h"

size_t arch_hash_func(uint64_t key, int seed, size_t size) {
    if (size == 0) {
        return 0;
    }

    // Keep deterministic scalar hash; runtime caps can tune diffusion constants.
    const bool has_aes = arch_cpu_has_system_any(ARCH_CPU_FEAT_X86_AES);
    const bool has_pclmul = arch_cpu_has_system_any(ARCH_CPU_FEAT_X86_PCLMUL);

    uint64_t x = key ^ (uint64_t)seed;
    x ^= x >> 30;
    x *= has_aes ? 0x9e3779b97f4a7c15ULL : 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= has_pclmul ? 0xc2b2ae3d27d4eb4fULL : 0x94d049bb133111ebULL;
    x ^= x >> 31;

    return (size_t)(x % size);
}
