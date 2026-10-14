#include "psa/crypto.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

void *malloc(size_t size);
void free(void *ptr);

void *mbedtls_platform_calloc_macro(size_t n, size_t size)
{
    size_t bytes = n * size;
    void *ptr = malloc(bytes);
    if (ptr) {
        memset(ptr, 0, bytes);
    }
    return ptr;
}

void mbedtls_platform_free_macro(void *ptr)
{
    if (ptr) {
        free(ptr);
    }
}

void mbedtls_zeroize_and_free(void *buf, size_t len)
{
    if (buf != NULL) {
        if (len > 0) {
            volatile unsigned char *p = (volatile unsigned char *)buf;
            while (len--) {
                *p++ = 0;
            }
        }
        free(buf);
    }
}
