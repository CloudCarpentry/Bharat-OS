#include <stdint.h>
#include <stddef.h>
#include "secure_boot.h"
#include "hal/hal_secure_boot.h"

/*
 * FPGA Bitstream Manager
 * Used for OpenRAN FEC (Forward Error Correction) offloading and
 * inline cryptographic acceleration.
 */

int __attribute__((weak)) fpga_mgr_platform_load_bitstream(uint32_t fpga_id,
                                                           const void* bitstream,
                                                           size_t size) {
    (void)fpga_id;
    (void)bitstream;
    (void)size;
    return -3; // Not implemented by platform driver
}

// Load an FPGA bitstream to dynamic reconfigurable hardware
int fpga_mgr_load_bitstream(uint32_t fpga_id, const void* bitstream, size_t size) {
    if (!bitstream || size == 0) return -1;

    const bharat_boot_policy_t* policy = bharat_boot_active_policy();

    // If secure boot is enforced, we need to lock the bitstream region
    // to prevent tampering while it's being verified and loaded.
    if (policy && policy->security_level == BHARAT_BOOT_SECURITY_ENFORCED) {
        // We assume bitstream is mapped to a contiguous physical address space.
        // BHARAT_SECURE_REGION_DEVICE_ONLY restricts the region for device access.
        if (bharat_secure_key_region_lock((uint64_t)bitstream, size, BHARAT_SECURE_REGION_DEVICE_ONLY) != 0) {
            // Failed to secure the memory region
            return -2;
        }
    }

    // Platform driver is expected to enforce secure boot/signature validation
    // and perform DMA/programming to the target configuration interface.
    int ret = fpga_mgr_platform_load_bitstream(fpga_id, bitstream, size);

    if (policy && policy->security_level == BHARAT_BOOT_SECURITY_ENFORCED) {
        // Unlock the region, scrubbing it to ensure key material/proprietary bitstream
        // isn't left lying around in RAM if it's no longer needed.
        bharat_secure_key_region_unlock((uint64_t)bitstream, size, BHARAT_SECURE_REGION_DEVICE_ONLY);
    }

    return ret;
}
