#include "boot/boot_security.h"
#include <stddef.h>

int boot_security_evaluate(const boot_info_t *bi, boot_security_decision_t *out_decision) {
    if (!bi || !out_decision) return -1;

    bool is_verified = (bi->security_state == BOOT_SECURITY_SECURE_VERIFIED ||
                        bi->security_state == BOOT_SECURITY_MEASURED_AND_VERIFIED) &&
                       bi->security_info.secure_boot_verified;

    switch (bi->selected_mode) {
        case BOOT_MODE_AUTOMOTIVE:
        case BOOT_MODE_NORMAL:
        case BOOT_MODE_SAFE:
            if (!is_verified) {
                *out_decision = BOOT_SEC_DECISION_DENY;
                return 0;
            }
            *out_decision = BOOT_SEC_DECISION_ALLOW;
            return 0;

        case BOOT_MODE_DEBUG:
        case BOOT_MODE_PROVISIONING:
            // Debug/provisioning modes shouldn't automatically bypass strict policy.
            // If they are allowed, they need to be verified.
            if (!is_verified) {
                *out_decision = BOOT_SEC_DECISION_DENY;
                return 0;
            }
            *out_decision = BOOT_SEC_DECISION_ALLOW;
            return 0;

        case BOOT_MODE_RECOVERY:
        case BOOT_MODE_SELFTEST:
        case BOOT_MODE_BENCHMARK:
        case BOOT_MODE_LEGACY_BRINGUP:
            if (!is_verified) {
                *out_decision = BOOT_SEC_DECISION_WARN_AND_ALLOW;
                return 0;
            }
            *out_decision = BOOT_SEC_DECISION_ALLOW;
            return 0;

        default:
            // Fail closed on unknown modes
            *out_decision = BOOT_SEC_DECISION_DENY;
            return -1;
    }
}

bool boot_security_allows_mode(const boot_info_t *bi, boot_mode_t mode) {
    if (!bi) return false;

    bool is_verified = (bi->security_state == BOOT_SECURITY_SECURE_VERIFIED ||
                        bi->security_state == BOOT_SECURITY_MEASURED_AND_VERIFIED) &&
                       bi->security_info.secure_boot_verified;

    switch (mode) {
        case BOOT_MODE_AUTOMOTIVE:
        case BOOT_MODE_NORMAL:
        case BOOT_MODE_SAFE:
        case BOOT_MODE_DEBUG:
        case BOOT_MODE_PROVISIONING:
            return is_verified;

        case BOOT_MODE_RECOVERY:
        case BOOT_MODE_SELFTEST:
        case BOOT_MODE_BENCHMARK:
        case BOOT_MODE_LEGACY_BRINGUP:
            return true;

        default:
            return false;
    }
}
