#ifndef BHARAT_MSG_FLAGS_H
#define BHARAT_MSG_FLAGS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Protocol Magic and Versioning
// ============================================================================

#define BHARAT_MSG_MAGIC         0x42485254  // "BHRT"
#define BHARAT_MSG_VERSION_MAJOR 1
#define BHARAT_MSG_VERSION_MINOR 0

// Minimum allowed length for the base header (44 bytes as defined in v1 spec)
#define BHARAT_MSG_HEADER_MIN_LEN 44

// ============================================================================
// Header Flag Masks
// ============================================================================

#define BHARAT_MSG_FLAG_REQUEST      (1U << 0)
#define BHARAT_MSG_FLAG_RESPONSE     (1U << 1)
#define BHARAT_MSG_FLAG_EVENT        (1U << 2)
#define BHARAT_MSG_FLAG_ERROR        (1U << 3)
#define BHARAT_MSG_FLAG_ACK_REQ      (1U << 4)
#define BHARAT_MSG_FLAG_RELIABLE     (1U << 5)
#define BHARAT_MSG_FLAG_FRAGMENTED   (1U << 6)
#define BHARAT_MSG_FLAG_HAS_CAPS     (1U << 7)
#define BHARAT_MSG_FLAG_HAS_OOL      (1U << 8)

// ============================================================================
// Flag Helpers
// ============================================================================

static inline int bharat_msg_is_request(uint32_t flags) {
    return (flags & BHARAT_MSG_FLAG_REQUEST) != 0;
}

static inline int bharat_msg_is_response(uint32_t flags) {
    return (flags & BHARAT_MSG_FLAG_RESPONSE) != 0;
}

static inline int bharat_msg_is_error(uint32_t flags) {
    return (flags & BHARAT_MSG_FLAG_ERROR) != 0;
}

static inline int bharat_msg_has_caps(uint32_t flags) {
    return (flags & BHARAT_MSG_FLAG_HAS_CAPS) != 0;
}

static inline int bharat_msg_has_ool(uint32_t flags) {
    return (flags & BHARAT_MSG_FLAG_HAS_OOL) != 0;
}

#ifdef __cplusplus
}
#endif

#endif // BHARAT_MSG_FLAGS_H
