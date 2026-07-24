#include <stdint.h>
#include <stddef.h>
#include "crc.h"

// Public API for CRC32 validation
// Standard IEEE 802.3 polynomial (Ethernet, gzip) = 0xEDB88320
//
// Dispatcher logic:
// We use the hardware accelerated backend only if its instructions
// match our exact IEEE 802.3 CRC variant.
// - ARM64 provides __crc32w which computes IEEE 802.3 CRC32, so we can use it.
// - x86_64 SSE4.2 ONLY provides instructions for CRC32C (Castagnoli). Using it
//   here would silently break wire compatibility, so we STRICTLY default to
//   the generic table-driven fallback on x86_64.
// - RISC-V Zbc is not unconditionally available in base profiles, falling back to generic.

uint32_t bharat_msg_crc32(const uint8_t *data, size_t len) {
#if defined(__aarch64__)
    // If we wanted to be perfectly safe, we'd dynamically check for ARMv8 CRC
    // extension support here. For this baseline, if we build for ARM64 with
    // hardware CRC enabled, we use it. Otherwise fallback.
    // In a real OS runtime, this would read an hwcap feature bit.
#ifdef BHARAT_ARM64_USE_HW_CRC
    return bharat_msg_crc32_aarch64(data, len);
#else
    return bharat_msg_crc32_generic(data, len);
#endif

#else
    // x86_64 and others (including RISC-V without explicit Zbc)
    return bharat_msg_crc32_generic(data, len);
#endif
}
