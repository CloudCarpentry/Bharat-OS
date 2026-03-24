#pragma once

#include <stdint.h>
#include <bharat/uapi/abi_types.h>
#include <bharat/uapi/crypto/contract.h>
#include <security/crypto_contract.h>

/**
 * @brief Initializes the Cryptographic Provider Registry.
 *
 * Sets up the internal linked list and any required spinlocks.
 * Must be called early in boot before hardware initialization.
 */
void crypto_registry_init(void);

/**
 * @brief Registers a new cryptographic hardware backend with the kernel.
 *
 * Hardware drivers (e.g., TPM, AES-NI) call this during probe to expose
 * their capabilities. The kernel assigns a unique ID.
 *
 * @param info Provider capabilities descriptor (e.g., name, backend type, ops).
 * @param ops The callback table the kernel will invoke.
 * @return >= 0 provider ID on success, or a negative SYS_E* error code.
 */
int crypto_provider_register(const crypto_provider_info_t *info, const crypto_provider_ops_t *ops);

/**
 * @brief Unregisters a cryptographic hardware backend.
 *
 * Typically called during driver unload or hardware hot-unplug.
 *
 * @param provider_id The ID returned by crypto_provider_register.
 * @return 0 on success, or SYS_ENOENT if not found.
 */
int crypto_provider_unregister(uint32_t provider_id);

/**
 * @brief Locates an active provider by its assigned ID.
 *
 * This does NOT perform a capability check; it is for internal kernel use.
 *
 * @param provider_id The ID of the provider.
 * @return The node descriptor, or NULL if not found.
 */
const crypto_provider_node_t* crypto_provider_find(uint32_t provider_id);

/**
 * @brief Capability-checked invocation of a provider's operation.
 *
 * This is the unified API for user-space (via syscall/IPC) or kernel subsystems
 * to request cryptographic operations. It verifies the caller holds the correct
 * capability for the requested operation BEFORE routing it to the hardware.
 *
 * @param provider_id The ID of the provider to use.
 * @param args The operation request parameters.
 * @return 0 on success, SYS_EPERM on capability denial, or a provider-specific error.
 */
int crypto_provider_invoke(uint32_t provider_id, crypto_op_args_t *args);

/**
 * @brief Convenience function for the RNG backend class.
 *
 * Uses the first registered `CRYPTO_BACKEND_RNG_PROVIDER` to fill the buffer,
 * enforcing `CAP_TYPE_RNG`.
 *
 * @param buf The buffer to fill.
 * @param len The size of the buffer.
 * @return 0 on success, or a negative SYS_E* error code.
 */
int crypto_get_random(void *buf, size_t len);

/**
 * @brief Zeroize a key buffer safely using compiler barriers.
 *
 * @param buf The buffer to clear.
 * @param len The length of the buffer.
 */
void crypto_zeroize_buffer(void *buf, size_t len);
