#ifndef BHARAT_ADDRESS_TOKEN_H
#define BHARAT_ADDRESS_TOKEN_H

#include <stdint.h>
#include "../security/isolation.h"

/*
 * Bharat-OS Address Tokens
 * Lightweight authorization mechanism for mapping privileged resources.
 */

#define ADDR_TOKEN_FLAG_VALID    (1U << 0)
#define ADDR_TOKEN_FLAG_REVOKED  (1U << 1)
#define ADDR_TOKEN_FLAG_EXPIRED  (1U << 2)
#define ADDR_TOKEN_FLAG_READ     (1U << 3)
#define ADDR_TOKEN_FLAG_WRITE    (1U << 4)
#define ADDR_TOKEN_FLAG_EXECUTE  (1U << 5)

typedef struct {
    uint32_t owner_id; // e.g., process_id or subsystem_id
    bharat_isolation_class_t iso_class;
    uint64_t resource_base; // Base address of the allowed resource (e.g., MMIO region)
    uint64_t resource_size; // Size of the allowed resource
    uint32_t flags; // Flags (Valid, Revoked, Expired, Read, Write, Execute)
    uint64_t expiry_ticks; // 0 if no expiry
} bharat_addr_token_t;

/*
 * Validate an address token for a specific operation.
 * Returns 0 if valid and authorized, negative error code otherwise.
 */
int bharat_addr_token_validate(const bharat_addr_token_t* token,
                               uint64_t addr,
                               uint64_t size,
                               uint32_t required_flags);

/*
 * Revoke an address token.
 */
void bharat_addr_token_revoke(bharat_addr_token_t* token);

#endif // BHARAT_ADDRESS_TOKEN_H
