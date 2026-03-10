#include "hal/hal_boot.h"

// Parse secure-boot trust evidence provided by firmware/bootloader

int boot_trust_verify_evidence(void) {
    bharat_boot_info_t* boot_info = hal_boot_get_info();
    if (!boot_info) return -1;

    // Apply strict policy if profile requires it
    if (boot_info->profile_toggles.strict_secure_boot_required) {
        if (boot_info->trust_evidence.trust_state != BHARAT_TRUST_VERIFIED) {
            // In a real scenario, panic here
            return -1;
        }
    }

    // Verify each previous stage supplied expected claims
    // Here we check the placeholder bitmap
    if (boot_info->trust_evidence.verified_stages_bitmap == 0 &&
        boot_info->trust_evidence.trust_state == BHARAT_TRUST_VERIFIED) {
        // Missing evidence for verified state
        return -1;
    }

    return 0; // Trust state accepted based on current policy
}
