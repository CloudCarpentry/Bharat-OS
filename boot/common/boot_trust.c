#include "hal/hal_boot.h"
#include "hal/hal_secure_boot.h"

// Parse secure-boot trust evidence provided by firmware/bootloader

static int boot_trust_try_refresh_from_hal(bharat_boot_info_t *boot_info) {
    bharat_boot_measurement_t measurements[8];
    size_t measurement_count = 8U;

    if (!boot_info) {
        return -1;
    }

    if (hal_secure_boot_get_measurements(measurements, &measurement_count) != 0) {
        return -1;
    }

    if (measurement_count == 0U) {
        return -1;
    }

    return hal_secure_boot_verify_measurements(
        measurements, measurement_count, &boot_info->trust_evidence);
}

int boot_trust_verify_evidence(void) {
    bharat_boot_info_t* boot_info = hal_boot_get_info();
    if (!boot_info) return -1;

    if (boot_info->trust_evidence.trust_state == BHARAT_TRUST_UNKNOWN) {
        (void)boot_trust_try_refresh_from_hal(boot_info);
    }

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
