#pragma once

#include <stdint.h>
#include <bharat/uapi/abi_types.h>

/**
 * @brief Formal Capability Types for Cryptography operations.
 *
 * In a capability-oriented microkernel, these define the exact rights a process
 * must hold to interact with the kernel's cryptographic mechanisms.
 */

/* Can list and open hardware cryptographic providers (e.g., TPM, AES-NI) */
#define CAP_TYPE_CRYPTO_PROVIDER 0x01

/* Can hold and reference a sealed key object in the kernel */
#define CAP_TYPE_CRYPTO_KEY      0x02

/* Can request hardware entropy from the RNG provider */
#define CAP_TYPE_RNG             0x04

/* Can bind/unbind data to the platform's trusted boot state */
#define CAP_TYPE_SEALER          0x08

/* Forward declarations for capability checking stubs */
typedef struct {
    uint32_t caps_mask;
} process_crypto_caps_t;

/**
 * @brief Simulates checking if the current process holds the required crypto capability.
 *
 * In a real implementation, this would interact with the kernel's formal
 * capability object system (e.g., capability.c). For this initial step,
 * we provide a stub interface.
 *
 * @param required_cap The capability type to check (e.g., CAP_TYPE_RNG).
 * @return 0 if the process holds the capability, -1 (or error code) otherwise.
 */
int crypto_cap_check(uint32_t required_cap);

/**
 * @brief Simulates granting a capability to the current process (for testing only).
 *
 * @param cap The capability type to grant.
 */
void crypto_cap_grant(uint32_t cap);

/**
 * @brief Simulates revoking a capability from the current process (for testing only).
 *
 * @param cap The capability type to revoke.
 */
void crypto_cap_revoke(uint32_t cap);
