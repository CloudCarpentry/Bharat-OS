#include "../../arch_crypto.h"
#include "arch/arch_cpu_caps.h"

bool arch_crypto_has_aes(void) {
    return arch_cpu_has_system_any(ARCH_CPU_FEAT_X86_AES);
}

bool arch_crypto_has_poly_mul(void) {
    return arch_cpu_has_system_any(ARCH_CPU_FEAT_X86_PCLMUL);
}

bool arch_crypto_accel_available(void) {
    return arch_crypto_has_aes() || arch_crypto_has_poly_mul();
}
