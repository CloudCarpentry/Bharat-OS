#pragma once

#include <stdint.h>
#include <bharat/uapi/abi_types.h>
#include <bharat/uapi/sys_errno.h>

/**
 * @brief Crypto provider operation classes.
 *
 * Defines the fundamental cryptographic operations a provider can support.
 */
typedef enum {
    CRYPTO_OP_GET_RANDOM    = 0,
    CRYPTO_OP_HASH_BUFFER   = 1,
    CRYPTO_OP_SEAL          = 2,
    CRYPTO_OP_UNSEAL        = 3,
    CRYPTO_OP_ZEROIZE_KEY   = 4,
    CRYPTO_OP_MAX
} crypto_op_class_t;

/**
 * @brief Provider backend classes.
 *
 * Maps to the architectural backend classes defined in hardware-backends.md.
 */
typedef enum {
    CRYPTO_BACKEND_CPU_ACCEL            = 1,
    CRYPTO_BACKEND_SECURE_ELEMENT_TPM   = 2,
    CRYPTO_BACKEND_RNG_PROVIDER         = 3,
} crypto_backend_class_t;

/**
 * @brief Crypto provider descriptor.
 *
 * Exposed to user-space so services can discover available hardware features.
 */
typedef struct {
    uint32_t provider_id;
    crypto_backend_class_t backend_class;
    uint32_t supported_ops_mask; /* Bitmask of (1 << crypto_op_class_t) */
    char name[32];               /* Null-terminated identifier (e.g., "aesni", "tpm2") */
} crypto_provider_info_t;

/**
 * @brief Generic arguments for cryptographic operations.
 */
typedef struct {
    crypto_op_class_t op;
    void* input_buf;
    size_t input_len;
    void* output_buf;
    size_t output_len;
    /* Optional context/key handle for seal/unseal */
    uint64_t key_handle;
} crypto_op_args_t;
