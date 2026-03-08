#include "arch/capabilities.h"

arch_capabilities_t g_arch_caps = {0};

void arch_capabilities_init(void) {
    // Basic runtime detection fallback (stubbed for now)
    g_arch_caps.has_avx2 = 0;
    g_arch_caps.has_fma = 0;
    g_arch_caps.has_aes = 0;
    g_arch_caps.has_vector = 0;
    g_arch_caps.has_crypto = 0;

#if defined(__x86_64__)
    // Minimal x86 cpuid check for AVX2 could go here
#elif defined(__aarch64__)
    // Read ID_AA64ISAR0_EL1 for AES, etc.
#elif defined(__riscv)
    // Read MISA or use hwprobe to set RVV / Crypto
#endif
}
