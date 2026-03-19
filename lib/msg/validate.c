#include "bharat/msg/validate.h"

int bharat_msg_header_validate(const bharat_msg_header_t* hdr, size_t max_mtu) {
    if (!hdr) {
        return BHARAT_MSG_ERR_MALFORMED_HEADER;
    }

    // Versioning
    if (hdr->version_major != BHARAT_MSG_VERSION_MAJOR) {
        return BHARAT_MSG_ERR_UNSUPPORTED_VER;
    }

    // Length sanity bounds
    if (hdr->header_len < BHARAT_MSG_HEADER_MIN_LEN) {
        return BHARAT_MSG_ERR_MALFORMED_HEADER;
    }

    if (hdr->total_len < hdr->header_len) {
        return BHARAT_MSG_ERR_MALFORMED_PAYLOAD;
    }

    if (max_mtu > 0 && hdr->total_len > max_mtu) {
        return BHARAT_MSG_ERR_TOO_LARGE;
    }

    // Check mutually exclusive message kinds (Request vs Response vs Event)
    uint32_t type_mask = BHARAT_MSG_FLAG_REQUEST | BHARAT_MSG_FLAG_RESPONSE | BHARAT_MSG_FLAG_EVENT;
    uint32_t set_types = hdr->flags & type_mask;

    // Only one core message type is allowed at a time.
    if (set_types != BHARAT_MSG_FLAG_REQUEST &&
        set_types != BHARAT_MSG_FLAG_RESPONSE &&
        set_types != BHARAT_MSG_FLAG_EVENT) {
        return BHARAT_MSG_ERR_MALFORMED_HEADER;
    }

    // Cap/Descriptor bounds sanity checking
    // Each capability descriptor is 34 bytes (0x22 bytes: 1+1+4+4+4+8+8+4)
    // Each OOL descriptor is 26 bytes (0x1A bytes: 1+1+8+8+8)
    size_t expected_cap_bytes = (size_t)hdr->cap_count * 34;
    size_t expected_ool_bytes = (size_t)hdr->desc_count * 26;

    if (expected_cap_bytes + expected_ool_bytes + hdr->header_len > hdr->total_len) {
        return BHARAT_MSG_ERR_MALFORMED_PAYLOAD;
    }

    // If caps count > 0, the HAS_CAPS flag must be set
    if (hdr->cap_count > 0 && !bharat_msg_has_caps(hdr->flags)) {
        return BHARAT_MSG_ERR_MALFORMED_PAYLOAD;
    }

    if (hdr->desc_count > 0 && !bharat_msg_has_ool(hdr->flags)) {
        return BHARAT_MSG_ERR_MALFORMED_PAYLOAD;
    }

    return BHARAT_MSG_OK;
}
