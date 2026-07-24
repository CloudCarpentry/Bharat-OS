#include "bharat/idl/ool.h"

int bharat_ool_encode(bharat_msg_builder_t* b, const bharat_ool_desc_t* desc) {
    if (!b || !desc) {
        return BHARAT_MSG_ERR_MALFORMED_PAYLOAD;
    }

    // An OOL descriptor is strictly 26 bytes (0x1A bytes). See msg-wire-format-v1.md.
    if (b->cap - b->off < 26) {
        return BHARAT_MSG_ERR_BUFFER_OVERFLOW;
    }

    bharat_msg_build_u8(b, desc->desc_type);
    bharat_msg_build_u8(b, desc->flags);
    bharat_msg_build_u64(b, desc->region_id);
    bharat_msg_build_u64(b, desc->offset);
    bharat_msg_build_u64(b, desc->length);

    return BHARAT_MSG_OK;
}

int bharat_ool_decode(bharat_msg_reader_t* r, bharat_ool_desc_t* desc) {
    if (!r || !desc) {
        return BHARAT_MSG_ERR_MALFORMED_PAYLOAD;
    }

    if (r->len - r->off < 26) {
        return BHARAT_MSG_ERR_TRUNCATED;
    }

    bharat_msg_read_u8(r, &desc->desc_type);
    bharat_msg_read_u8(r, &desc->flags);
    bharat_msg_read_u64(r, &desc->region_id);
    bharat_msg_read_u64(r, &desc->offset);
    bharat_msg_read_u64(r, &desc->length);

    return BHARAT_MSG_OK;
}
