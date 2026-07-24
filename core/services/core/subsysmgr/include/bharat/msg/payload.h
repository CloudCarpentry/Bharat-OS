#ifndef BHARAT_MSG_PAYLOAD_H
#define BHARAT_MSG_PAYLOAD_H

#include "types.h"
#include "errors.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Bounded Codec Types
// ============================================================================

// A reader strictly bound to an incoming buffer length.
typedef struct {
    const uint8_t* buf;
    size_t len;
    size_t off;
} bharat_msg_reader_t;

// A writer strictly bound to an outgoing buffer capacity.
typedef struct {
    uint8_t* buf;
    size_t cap;
    size_t off;
} bharat_msg_builder_t;

// ============================================================================
// Initialization
// ============================================================================

static inline void bharat_msg_reader_init(bharat_msg_reader_t* r, const uint8_t* buf, size_t len, size_t initial_offset) {
    r->buf = buf;
    r->len = len;
    r->off = initial_offset;
}

static inline void bharat_msg_builder_init(bharat_msg_builder_t* b, uint8_t* buf, size_t cap, size_t initial_offset) {
    b->buf = buf;
    b->cap = cap;
    b->off = initial_offset;
}

// ============================================================================
// Codec API signatures (implemented in encode.c / decode.c)
// ============================================================================

// ---- Builders (Encode) ----
int bharat_msg_build_u8(bharat_msg_builder_t* b, uint8_t v);
int bharat_msg_build_u16(bharat_msg_builder_t* b, uint16_t v);
int bharat_msg_build_u32(bharat_msg_builder_t* b, uint32_t v);
int bharat_msg_build_u64(bharat_msg_builder_t* b, uint64_t v);
int bharat_msg_build_bool(bharat_msg_builder_t* b, bool v);

int bharat_msg_build_bytes(bharat_msg_builder_t* b, const uint8_t* data, uint32_t len);
int bharat_msg_build_string(bharat_msg_builder_t* b, const char* str);

// ---- Readers (Decode) ----
int bharat_msg_read_u8(bharat_msg_reader_t* r, uint8_t* out);
int bharat_msg_read_u16(bharat_msg_reader_t* r, uint16_t* out);
int bharat_msg_read_u32(bharat_msg_reader_t* r, uint32_t* out);
int bharat_msg_read_u64(bharat_msg_reader_t* r, uint64_t* out);
int bharat_msg_read_bool(bharat_msg_reader_t* r, bool* out);

// Bounded readers (Copy data off the wire into inline destination buffers)
int bharat_msg_read_bytes_bounded(bharat_msg_reader_t* r, uint8_t* dst, uint32_t dst_max, uint32_t* out_len);
int bharat_msg_read_string_bounded(bharat_msg_reader_t* r, char* dst, uint32_t dst_max, uint32_t* out_len);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_MSG_PAYLOAD_H
