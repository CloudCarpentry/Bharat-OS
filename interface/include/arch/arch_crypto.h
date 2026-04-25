#ifndef BHARAT_ARCH_CRYPTO_H
#define BHARAT_ARCH_CRYPTO_H

#include <stdbool.h>

bool arch_crypto_has_aes(void);
bool arch_crypto_has_poly_mul(void);
bool arch_crypto_accel_available(void);

#endif
