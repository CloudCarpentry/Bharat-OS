#include "../arch_crypto.h"

__attribute__((weak)) bool arch_crypto_has_aes(void) {
    return false;
}

__attribute__((weak)) bool arch_crypto_has_poly_mul(void) {
    return false;
}

__attribute__((weak)) bool arch_crypto_accel_available(void) {
    return false;
}
