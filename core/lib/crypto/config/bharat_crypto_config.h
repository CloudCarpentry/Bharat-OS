#ifndef BHARAT_CRYPTO_CONFIG_H
#define BHARAT_CRYPTO_CONFIG_H

/* PSA algorithms - minimal scope */
#define PSA_WANT_ALG_SHA_256 1
#define PSA_WANT_ALG_HMAC 1
#define PSA_WANT_ALG_HKDF 1
#define PSA_WANT_ALG_GCM 1
#define PSA_WANT_KEY_TYPE_AES 1

/* MBEDTLS components for minimal scope */
#define MBEDTLS_PSA_CRYPTO_C
#define MBEDTLS_PLATFORM_ZEROIZE_ALT

/* Disable specific modules that are pulled in by default */
#undef MBEDTLS_SHA1_C
#undef MBEDTLS_SHA512_C
#undef MBEDTLS_SHA3_C
#undef MBEDTLS_CMAC_C
#undef MBEDTLS_AESCE_C

/* Ensure we don't accidentally fall back to dummy entropy in production */
#define MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG

/* Disable ITS which needs files */
#undef MBEDTLS_PSA_CRYPTO_STORAGE_C
#undef MBEDTLS_PSA_ITS_FILE_C

/* Disable tests that causes printf linking errors */
#undef MBEDTLS_SELF_TEST
#undef MBEDTLS_FS_IO

/* Freestanding configurations */
#ifndef MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_C
#endif
#ifndef MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_PLATFORM_MEMORY
#endif
#ifndef MBEDTLS_PLATFORM_NO_STD_FUNCTIONS
#define MBEDTLS_PLATFORM_NO_STD_FUNCTIONS
#endif

#undef MBEDTLS_HAVE_TIME
#undef MBEDTLS_HAVE_TIME_DATE

/* Explicitly disable NV seed handling which we don't support */
#undef MBEDTLS_ENTROPY_NV_SEED
#undef MBEDTLS_PLATFORM_NV_SEED_ALT

#endif
#undef MBEDTLS_HAVE_ASM
#undef MBEDTLS_HAVE_ASM
