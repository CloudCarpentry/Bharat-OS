#include <stdint.h>
#include <stddef.h>

// ARM64 HW Accelerated Backend for IEEE 802.3 CRC32
//
// ARMv8 provides the `__crc32*` series of instructions.
// Note carefully: ARM provides both __crc32w (IEEE) and __crc32cw (Castagnoli).
// Since our wire protocol expects IEEE 802.3 (Polynomial 0xEDB88320),
// we strictly use the __crc32w variants.

#if defined(__aarch64__)
#include <arm_acle.h>

uint32_t bharat_msg_crc32_aarch64(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;

    // Process 8 bytes at a time
    while (len >= 8) {
        crc = __crc32d(crc, *(const uint64_t *)data);
        data += 8;
        len -= 8;
    }

    // Process 4 bytes
    if (len >= 4) {
        crc = __crc32w(crc, *(const uint32_t *)data);
        data += 4;
        len -= 4;
    }

    // Process 2 bytes
    if (len >= 2) {
        crc = __crc32h(crc, *(const uint16_t *)data);
        data += 2;
        len -= 2;
    }

    // Process remaining byte
    if (len == 1) {
        crc = __crc32b(crc, *data);
    }

    return crc ^ 0xFFFFFFFF;
}

#endif // __aarch64__
