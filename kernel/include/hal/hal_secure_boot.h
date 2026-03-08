#ifndef BHARAT_OS_HAL_SECURE_BOOT_H
#define BHARAT_OS_HAL_SECURE_BOOT_H

#include "secure_boot.h"

/**
 * HAL/Board interface for Secure Boot.
 * Allows each architecture and board to provide specialized policy discovery
 * and architecture-specific hardware security checks (e.g., TrustZone, PMP,
 * SGX).
 */

/**
 * Boards must implement this to provide the active boot policy.
 * If not implemented by a board, a default generic policy may be used.
 */
const bharat_boot_policy_t *hal_board_get_boot_policy(void);

/**
 * Architecture-specific hardware security check.
 * Called during early boot to verify that the specified policy can be enforced
 * by the current CPU/SoC features.
 */
int hal_secure_boot_arch_check(const bharat_boot_policy_t *policy);

#endif // BHARAT_OS_HAL_SECURE_BOOT_H
