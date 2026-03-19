#include "bharat/msg/payload.h"

int bharat_msg_read_u8(bharat_msg_reader_t* r, uint8_t* out) {
    if (r->len - r->off < sizeof(uint8_t)) {
        return BHARAT_MSG_ERR_TRUNCATED;
    }
    *out = r->buf[r->off++];
    return BHARAT_MSG_OK;
}

int bharat_msg_read_u16(bharat_msg_reader_t* r, uint16_t* out) {
    if (r->len - r->off < sizeof(uint16_t)) {
        return BHARAT_MSG_ERR_TRUNCATED;
    }
    *out = bharat_load_le16(r->buf + r->off);
    r->off += sizeof(uint16_t);
    return BHARAT_MSG_OK;
}

int bharat_msg_read_u32(bharat_msg_reader_t* r, uint32_t* out) {
    if (r->len - r->off < sizeof(uint32_t)) {
        return BHARAT_MSG_ERR_TRUNCATED;
    }
    *out = bharat_load_le32(r->buf + r->off);
    r->off += sizeof(uint32_t);
    return BHARAT_MSG_OK;
}

int bharat_msg_read_u64(bharat_msg_reader_t* r, uint64_t* out) {
    if (r->len - r->off < sizeof(uint64_t)) {
        return BHARAT_MSG_ERR_TRUNCATED;
    }
    *out = bharat_load_le64(r->buf + r->off);
    r->off += sizeof(uint64_t);
    return BHARAT_MSG_OK;
}

int bharat_msg_read_bool(bharat_msg_reader_t* r, bool* out) {
    uint8_t val;
    int rc = bharat_msg_read_u8(r, &val);
    if (rc == BHARAT_MSG_OK) {
        *out = (val != 0);
    }
    return rc;
}

int bharat_msg_read_bytes_bounded(bharat_msg_reader_t* r, uint8_t* dst, uint32_t dst_max, uint32_t* out_len) {
    uint32_t len;
    int rc = bharat_msg_read_u32(r, &len);
    if (rc != BHARAT_MSG_OK) {
        return rc;
    }

    if (len > dst_max) {
        return BHARAT_MSG_ERR_BUFFER_OVERFLOW;
    }

    if (r->len - r->off < len) {
        return BHARAT_MSG_ERR_TRUNCATED;
    }

    if (len > 0) {
        for (uint32_t i = 0; i < len; i++) {
            dst[i] = (r->buf + r->off)[i];
        }
        r->off += len;
    }
    *out_len = len;
    return BHARAT_MSG_OK;
}

int bharat_msg_read_string_bounded(bharat_msg_reader_t* r, char* dst, uint32_t dst_max, uint32_t* out_len) {
    // A string is just encoded as length-prefixed bytes
    uint32_t len;
    int rc = bharat_msg_read_bytes_bounded(r, (uint8_t*)dst, dst_max - 1, &len); // reserve 1 byte for null terminator
    if (rc != BHARAT_MSG_OK) {
        return rc;
    }

    // Always null terminate strings for safe C-layer usage
    dst[len] = '\0';
    if (out_len) {
        *out_len = len;
    }
    return BHARAT_MSG_OK;
}
