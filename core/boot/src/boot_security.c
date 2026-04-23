#include "boot/boot_security.h"
#include <stddef.h>

int boot_security_evaluate(const boot_info_t *bi, boot_security_decision_t *out_decision) {
    if (!bi || !out_decision) return -1;

    // For now, implement simple policy logic
    // A production system might look for PROFILE_DESKTOP / PROFILE_SERVER
    // and explicitly require secure boot if a strict config is set.

    if (bi->selected_mode == BOOT_MODE_RECOVERY) {
        // Recovery might allow insecure boot but we should log/warn
        if (!bi->security_info.secure_boot_verified) {
            *out_decision = BOOT_SEC_DECISION_WARN_AND_ALLOW;
            return 0;
        }
    }

    // Default to allow in this foundational patch unless we explicitly set a profile
    // that rejects it.
    *out_decision = BOOT_SEC_DECISION_ALLOW;

    return 0;
}

bool boot_security_allows_mode(const boot_info_t *bi, boot_mode_t mode) {
    if (!bi) return false;

    if (mode == BOOT_MODE_DEBUG || mode == BOOT_MODE_PROVISIONING) {
        // We might want to restrict these modes to only when secure boot is disabled
        // or a specific debug cert is enrolled.
        // For now, we allow.
        return true;
    }

    if (mode == BOOT_MODE_NORMAL) {
        // We expect normal to be the steady state.
        return true;
    }

    return true;
}
