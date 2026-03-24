#pragma once

#include <stdint.h>
#include <bharat/uapi/abi_types.h>
#include <bharat/uapi/crypto/contract.h>
#include <bharat/uapi/sys_errno.h>
#include <security/crypto_caps.h>

/**
 * @brief Kernel-side Provider Operations Table.
 *
 * Hardware drivers (e.g., TPM, AES-NI) must implement this struct
 * and register it with the kernel's cryptographic registry.
 */
typedef struct {
    /**
     * @brief Handle an arbitrary cryptographic operation.
     *
     * Implementations must validate inputs carefully and ensure they
     * do not execute operations they are not authorized for or capable of.
     *
     * @param args The operation request parameters.
     * @return 0 on success, or a SYS_E* error code (e.g., SYS_ENOTSUP).
     */
    int (*invoke)(crypto_op_args_t *args);

    /**
     * @brief Securely clear a buffer.
     *
     * This must be implemented using compiler-safe primitives
     * (e.g., memset_explicit) to prevent optimization from removing it.
     */
    void (*zeroize)(void *buf, size_t len);

    /**
     * @brief Retrieve high-quality entropy from the hardware source.
     *
     * Only valid for RNG_PROVIDER backends.
     */
    int (*get_random)(void *buf, size_t len);

} crypto_provider_ops_t;

/**
 * @brief Kernel Provider Registration Descriptor.
 *
 * Represents an active hardware cryptographic backend registered
 * within the kernel.
 */
typedef struct crypto_provider_node {
    uint32_t id;
    crypto_provider_info_t info;
    const crypto_provider_ops_t *ops;

    struct crypto_provider_node *next;
} crypto_provider_node_t;
