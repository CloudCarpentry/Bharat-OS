#include "bharat/idl/capwire.h"

// ============================================================================
// Encoding/Decoding (Wire representation)
// ============================================================================

int bharat_capwire_encode(bharat_msg_builder_t* b, const bharat_capwire_desc_t* desc) {
    if (!b || !desc) {
        return BHARAT_MSG_ERR_MALFORMED_PAYLOAD;
    }

    // A Capwire Descriptor is strictly 34 bytes (0x22). See msg-wire-format-v1.md.
    if (b->cap - b->off < 34) {
        return BHARAT_MSG_ERR_BUFFER_OVERFLOW;
    }

    bharat_msg_build_u8(b, desc->cap_type);
    bharat_msg_build_u8(b, desc->transfer_mode);
    bharat_msg_build_u32(b, desc->rights_mask);
    bharat_msg_build_u32(b, desc->origin_node);
    bharat_msg_build_u32(b, desc->issuer_id);
    bharat_msg_build_u64(b, desc->object_id);
    bharat_msg_build_u64(b, desc->nonce);
    bharat_msg_build_u32(b, desc->generation);

    return BHARAT_MSG_OK;
}

int bharat_capwire_decode(bharat_msg_reader_t* r, bharat_capwire_desc_t* desc) {
    if (!r || !desc) {
        return BHARAT_MSG_ERR_MALFORMED_PAYLOAD;
    }

    if (r->len - r->off < 34) {
        return BHARAT_MSG_ERR_TRUNCATED;
    }

    bharat_msg_read_u8(r, &desc->cap_type);
    bharat_msg_read_u8(r, &desc->transfer_mode);
    bharat_msg_read_u32(r, &desc->rights_mask);
    bharat_msg_read_u32(r, &desc->origin_node);
    bharat_msg_read_u32(r, &desc->issuer_id);
    bharat_msg_read_u64(r, &desc->object_id);
    bharat_msg_read_u64(r, &desc->nonce);
    bharat_msg_read_u32(r, &desc->generation);

    return BHARAT_MSG_OK;
}
