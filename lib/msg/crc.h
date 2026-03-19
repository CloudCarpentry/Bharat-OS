#ifndef BHARAT_LIB_MSG_CRC_H
#define BHARAT_LIB_MSG_CRC_H

#include <stdint.h>
#include <stddef.h>

// Internal generic implementation
uint32_t bharat_msg_crc32_generic(const uint8_t *data, size_t len);

// Arch-specific internal declarations (if enabled)
#if defined(__aarch64__)
uint32_t bharat_msg_crc32_aarch64(const uint8_t *data, size_t len);
#endif

// We deliberately do not define an x86_64 prototype here because
// x86 SSE4.2 CRC32 instructions compute CRC32C (Castagnoli), which
// does NOT match the IEEE 802.3 CRC32 polynomial used by bharat_msg_crc32.
// Doing so would break wire compatibility. We use the generic fallback for x86_64.

#endif // BHARAT_LIB_MSG_CRC_H
