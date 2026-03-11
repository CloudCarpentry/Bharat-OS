#ifndef BHARAT_OS_HAL_SECURE_BOOT_H
#define BHARAT_OS_HAL_SECURE_BOOT_H

#include "secure_boot.h"
#include <stdint.h>
#include <stdbool.h>

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


// NEW BHARAT MULTIKERNEL TRUST EVIDENCE AND POLICY TOGGLES

typedef enum {
    BHARAT_TRUST_UNKNOWN = 0,
    BHARAT_TRUST_UNSIGNED,
    BHARAT_TRUST_MEASURED,
    BHARAT_TRUST_VERIFIED
} bharat_boot_trust_state_t;

typedef struct {
    bharat_boot_trust_state_t trust_state;
    uint32_t verified_stages_bitmap;
    uint8_t measurements_digest[32]; // Basic SHA-256 placeholder
    uint32_t policy_decision_record;
} bharat_trust_evidence_t;

typedef struct {
    bool smp_allowed;
    bool strict_secure_boot_required;
    bool unsigned_module_loading_disabled;
    bool panic_policy_strict;
    bool timer_preference_oneshot;
} bharat_profile_toggles_t;

#endif // BHARAT_OS_HAL_SECURE_BOOT_H
