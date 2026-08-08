#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
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

static crc_func_t g_crc_backend = bharat_msg_crc32_generic;
static bool g_crc_backend_registered = false;

void bharat_msg_crc_register_backend(crc_func_t backend) {
    if (!g_crc_backend_registered && backend != NULL) {
        g_crc_backend = backend;
        g_crc_backend_registered = true;
    }
}

uint32_t bharat_msg_crc32(const uint8_t *data, size_t len) {
    return g_crc_backend(data, len);
}
