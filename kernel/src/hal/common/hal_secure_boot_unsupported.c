#include "hal/hal_secure_boot.h"
#include <stddef.h>

/*
 * Generic/Unsupported Implementation of Secure Boot and Crypto Memory Isolation.
 * Used for edge devices (arm32, riscv32, arm-m) that may not support hardware
 * roots of trust, TrustZone, PMP, SGX/TDX, etc.
 */

int hal_secure_boot_get_measurements(bharat_boot_measurement_t *out_measurements,
                                     size_t *inout_count) {
    (void)out_measurements;
    (void)inout_count;
    // -1 indicates unsupported or no measurements available
    return -1;
}

int hal_secure_boot_verify_measurements(
    const bharat_boot_measurement_t *measurements, size_t count,
    bharat_trust_evidence_t *out_evidence) {
    (void)measurements;
    (void)count;
    (void)out_evidence;
    return -1;
}

int hal_secure_mem_restrict_region(const bharat_secure_region_t *region) {
    (void)region;
    // No hardware-backed memory restriction available on this platform.
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
