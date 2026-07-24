#ifndef BHARAT_MSG_TYPES_H
#define BHARAT_MSG_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Endian Helpers (Always encode little-endian on the wire)
// ============================================================================

static inline uint16_t bharat_load_le16(const uint8_t* p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static inline uint32_t bharat_load_le32(const uint8_t* p) {
    return (uint32_t)((uint32_t)p[0] | ((uint32_t)p[1] << 8) |
                      ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24));
}

static inline uint64_t bharat_load_le64(const uint8_t* p) {
    uint32_t lo = bharat_load_le32(p);
    uint32_t hi = bharat_load_le32(p + 4);
    return ((uint64_t)hi << 32) | lo;
}

static inline void bharat_store_le16(uint8_t* p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
}

static inline void bharat_store_le32(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
    p[2] = (uint8_t)((v >> 16) & 0xFF);
    p[3] = (uint8_t)((v >> 24) & 0xFF);
}

static inline void bharat_store_le64(uint8_t* p, uint64_t v) {
    bharat_store_le32(p, (uint32_t)(v & 0xFFFFFFFF));
    bharat_store_le32(p + 4, (uint32_t)((v >> 32) & 0xFFFFFFFF));
}

#ifdef __cplusplus
}
#endif

#endif // BHARAT_MSG_TYPES_H
