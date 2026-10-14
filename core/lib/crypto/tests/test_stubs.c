#include <stddef.h>
#include <stdint.h>

int memcmp(const void *str1, const void *str2, size_t count) {
    const unsigned char *s1 = (const unsigned char *)str1;
    const unsigned char *s2 = (const unsigned char *)str2;
    while (count-- > 0) {
        if (*s1++ != *s2++)
            return s1[-1] < s2[-1] ? -1 : 1;
    }
    return 0;
}
void *memmove(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--) *--d = *--s;
    }
    return dest;
}

void bharat_runtime_init(void) {}
void bharat_runtime_log(const char *msg) {}

/* Missing symbols that might not be in builtin drivers depending on configuration */
int mbedtls_psa_xof_abort(void) { return 0; }
int mbedtls_psa_xof_setup(void) { return 0; }
int mbedtls_psa_xof_set_context(void) { return 0; }
int mbedtls_psa_xof_update(void) { return 0; }
int mbedtls_psa_xof_output(void) { return 0; }

int mbedtls_asn1_write_len(void) { return 0; }
int mbedtls_asn1_write_tag(void) { return 0; }
int mbedtls_asn1_get_tag(void) { return 0; }
int mbedtls_asn1_get_int(void) { return 0; }
int mbedtls_asn1_write_int(void) { return 0; }
int mbedtls_oid_get_oid_by_md(void) { return 0; }
