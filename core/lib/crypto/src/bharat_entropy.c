#include "psa/crypto.h"
#include "psa/crypto_extra.h"
#include <string.h>

void mbedtls_platform_zeroize(void *buf, size_t len)
{
    if (buf == NULL || len == 0) {
        return;
    }
    volatile unsigned char *p = (volatile unsigned char *)buf;
    while (len--) {
        *p++ = 0;
    }
}

psa_status_t mbedtls_psa_external_get_random(
    mbedtls_psa_external_random_context_t *context,
    uint8_t *output, size_t output_size, size_t *output_length)
{
    (void)context;
    (void)output;
    (void)output_size;
    (void)output_length;
    /* Return insufficient entropy since we are failing closed */
    return PSA_ERROR_INSUFFICIENT_ENTROPY;
}
