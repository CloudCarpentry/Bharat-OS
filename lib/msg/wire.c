#include "bharat/msg/wire.h"

// Provide an internal memset definition for the freestanding environment.
static void *internal_memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

// ============================================================================
// Protocol Decoding
// ============================================================================

int bharat_msg_header_decode(const uint8_t* buf, size_t buf_len, bharat_msg_header_t* hdr) {
    if (!buf || !hdr) {
        return BHARAT_MSG_ERR_MALFORMED_HEADER;
    }

    if (buf_len < BHARAT_MSG_HEADER_MIN_LEN) {
        return BHARAT_MSG_ERR_TRUNCATED;
    }

    // Verify magic
    uint32_t magic = bharat_load_le32(buf + 0x00);
    if (magic != BHARAT_MSG_MAGIC) {
        return BHARAT_MSG_ERR_MALFORMED_HEADER;
    }

    // Decode directly into the normalized structure
    hdr->version_major = buf[0x04];
    hdr->version_minor = buf[0x05];
    hdr->header_len    = bharat_load_le16(buf + 0x06);
    hdr->service_id    = bharat_load_le16(buf + 0x08);
    hdr->opcode        = bharat_load_le16(buf + 0x0A);
    hdr->flags         = bharat_load_le32(buf + 0x0C);
    hdr->total_len     = bharat_load_le32(buf + 0x10);
    hdr->request_id    = bharat_load_le64(buf + 0x14);
    hdr->src_node      = bharat_load_le32(buf + 0x1C);
    hdr->dst_node      = bharat_load_le32(buf + 0x20);
    hdr->cap_count     = bharat_load_le16(buf + 0x24);
    hdr->desc_count    = bharat_load_le16(buf + 0x26);
    hdr->header_crc    = bharat_load_le32(buf + 0x28);

    // Initial length boundaries
    if (hdr->header_len < BHARAT_MSG_HEADER_MIN_LEN) {
        return BHARAT_MSG_ERR_MALFORMED_HEADER;
    }

    if (buf_len < hdr->header_len) {
        return BHARAT_MSG_ERR_TRUNCATED;
    }

    // Must be supported version
    if (hdr->version_major != BHARAT_MSG_VERSION_MAJOR) {
        return BHARAT_MSG_ERR_UNSUPPORTED_VER;
    }

    return BHARAT_MSG_OK;
}

// ============================================================================
// Protocol Encoding
// ============================================================================

int bharat_msg_header_encode(const bharat_msg_header_t* hdr, uint8_t* buf, size_t buf_len) {
    if (!hdr || !buf) {
        return BHARAT_MSG_ERR_MALFORMED_HEADER;
    }

    if (buf_len < hdr->header_len || buf_len < BHARAT_MSG_HEADER_MIN_LEN) {
        return BHARAT_MSG_ERR_TOO_LARGE; // Actually, the output buffer is too small
    }

    // Write Magic (always BHRT)
    bharat_store_le32(buf + 0x00, BHARAT_MSG_MAGIC);

    // Write Version
    buf[0x04] = hdr->version_major;
    buf[0x05] = hdr->version_minor;

    // Remaining basic scalar fields
    bharat_store_le16(buf + 0x06, hdr->header_len);
    bharat_store_le16(buf + 0x08, hdr->service_id);
    bharat_store_le16(buf + 0x0A, hdr->opcode);
    bharat_store_le32(buf + 0x0C, hdr->flags);
    bharat_store_le32(buf + 0x10, hdr->total_len);
    bharat_store_le64(buf + 0x14, hdr->request_id);
    bharat_store_le32(buf + 0x1C, hdr->src_node);
    bharat_store_le32(buf + 0x20, hdr->dst_node);
    bharat_store_le16(buf + 0x24, hdr->cap_count);
    bharat_store_le16(buf + 0x26, hdr->desc_count);
    bharat_store_le32(buf + 0x28, hdr->header_crc);

    // Future-proof: Zero-out any remaining padding bytes up to header_len
    if (hdr->header_len > BHARAT_MSG_HEADER_MIN_LEN) {
        internal_memset(buf + BHARAT_MSG_HEADER_MIN_LEN, 0, hdr->header_len - BHARAT_MSG_HEADER_MIN_LEN);
    }

    return BHARAT_MSG_OK;
}
