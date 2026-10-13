#include "hal/hal_secure_boot.h"
#include <stddef.h>

/*
 * ARM64 Implementation of Secure Boot and Crypto Memory Isolation.
 * This file provides placeholders for ARM TrustZone SMC calls and EFI/FDT
 * measurement retrieval.
 */

int hal_secure_boot_get_measurements(bharat_boot_measurement_t *out_measurements,
                                     size_t *inout_count) {
    (void)out_measurements;
    if (inout_count) {
        *inout_count = 0;
    }
    return -1;
}

int hal_secure_boot_verify_measurements(
    const bharat_boot_measurement_t *measurements, size_t count,
    bharat_trust_evidence_t *out_evidence) {
    (void)measurements;
    (void)count;
    if (out_evidence) {
        out_evidence->trust_state = BHARAT_TRUST_UNKNOWN;
        out_evidence->verified_stages_bitmap = 0;
    }
    return -1;
}

int hal_secure_mem_restrict_region(const bharat_secure_region_t *region) {
    (void)region;
    return -1;
}

int hal_secure_mem_release_region(const bharat_secure_region_t *region) {
    (void)region;
    return -1;
}

int hal_secure_crypto_dma_window_config(const bharat_crypto_dma_window_t *window) {
    (void)window;
    return -1;
}

int hal_secure_crypto_dma_window_clear(const bharat_crypto_dma_window_t *window) {
    (void)window;
    return -1;
}
