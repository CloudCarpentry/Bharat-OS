#include "bharat/msg/payload.h"

int bharat_msg_build_u8(bharat_msg_builder_t* b, uint8_t v) {
    if (b->cap - b->off < sizeof(uint8_t)) {
        return BHARAT_MSG_ERR_BUFFER_OVERFLOW;
    }
    b->buf[b->off++] = v;
    return BHARAT_MSG_OK;
}

int bharat_msg_build_u16(bharat_msg_builder_t* b, uint16_t v) {
    if (b->cap - b->off < sizeof(uint16_t)) {
        return BHARAT_MSG_ERR_BUFFER_OVERFLOW;
    }
    bharat_store_le16(b->buf + b->off, v);
    b->off += sizeof(uint16_t);
    return BHARAT_MSG_OK;
}

int bharat_msg_build_u32(bharat_msg_builder_t* b, uint32_t v) {
    if (b->cap - b->off < sizeof(uint32_t)) {
        return BHARAT_MSG_ERR_BUFFER_OVERFLOW;
    }
    bharat_store_le32(b->buf + b->off, v);
    b->off += sizeof(uint32_t);
    return BHARAT_MSG_OK;
}

int bharat_msg_build_u64(bharat_msg_builder_t* b, uint64_t v) {
    if (b->cap - b->off < sizeof(uint64_t)) {
        return BHARAT_MSG_ERR_BUFFER_OVERFLOW;
    }
    bharat_store_le64(b->buf + b->off, v);
    b->off += sizeof(uint64_t);
    return BHARAT_MSG_OK;
}

int bharat_msg_build_bool(bharat_msg_builder_t* b, bool v) {
    return bharat_msg_build_u8(b, v ? 1 : 0);
}

int bharat_msg_build_bytes(bharat_msg_builder_t* b, const uint8_t* data, uint32_t len) {
    // 4-byte length prefix + actual bytes
    if (b->cap - b->off < sizeof(uint32_t) + len) {
        return BHARAT_MSG_ERR_BUFFER_OVERFLOW;
    }
    bharat_store_le32(b->buf + b->off, len);
    b->off += sizeof(uint32_t);

    if (len > 0) {
        for (uint32_t i = 0; i < len; i++) {
            (b->buf + b->off)[i] = data[i];
        }
        b->off += len;
    }
    return BHARAT_MSG_OK;
}

int bharat_msg_build_string(bharat_msg_builder_t* b, const char* str) {
    uint32_t len = 0;
    if (str) {
        while (str[len] != '\0') len++;
    }
    return bharat_msg_build_bytes(b, (const uint8_t*)str, len);
}
