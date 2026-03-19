#ifndef BHARAT_MSG_WIRE_H
#define BHARAT_MSG_WIRE_H

#include "bharat/msg/types.h"
#include "bharat/msg/flags.h"
#include "bharat/msg/errors.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Normalized Internal Header Structure
// ============================================================================
// Used within the host environment. Must not be overlayed directly on buffers.
// ============================================================================

typedef struct {
    uint8_t  version_major;
    uint8_t  version_minor;
    uint16_t header_len;
    uint16_t service_id;
    uint16_t opcode;
    uint32_t flags;
    uint32_t total_len;
    uint64_t request_id;
    uint32_t src_node;
    uint32_t dst_node;
    uint16_t cap_count;
    uint16_t desc_count;
    uint32_t header_crc;
} bharat_msg_header_t;

// ============================================================================
// Codec Signatures
// ============================================================================

/**
 * @brief Decode the raw bytes from `buf` into the normalized `hdr`.
 *
 * Enforces basic structural constraints (length boundaries).
 *
 * @param buf Raw incoming buffer bytes.
 * @param buf_len Total available bytes in buffer.
 * @param hdr Output normalized struct.
 * @return BHARAT_MSG_OK on success, negative error code otherwise.
 */
int bharat_msg_header_decode(const uint8_t* buf, size_t buf_len, bharat_msg_header_t* hdr);

/**
 * @brief Encode the normalized `hdr` into `buf`.
 *
 * @param hdr The normalized internal header struct.
 * @param buf Pre-allocated buffer to hold at least `hdr->header_len` bytes.
 * @param buf_len Available size in `buf`.
 * @return BHARAT_MSG_OK on success, BHARAT_MSG_ERR_TOO_LARGE if buf is small.
 */
int bharat_msg_header_encode(const bharat_msg_header_t* hdr, uint8_t* buf, size_t buf_len);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_MSG_WIRE_H
