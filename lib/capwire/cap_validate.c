#include "bharat/idl/capwire.h"

// ============================================================================
// Protocol Decoding
// ============================================================================

int bharat_capwire_validate(const bharat_capwire_desc_t* desc) {
    if (!desc) {
        return BHARAT_MSG_ERR_MALFORMED_PAYLOAD;
    }

    if (desc->transfer_mode > BHARAT_CAP_XFER_DELEGATE_ATTENUATED) {
        return BHARAT_MSG_ERR_INVALID_CAP; // Unknown transfer semantic
    }

    // A capability must originate from somewhere and be over something.
    // 0 is often considered a null/invalid ID, so let's reject completely null tokens
    if (desc->origin_node == 0 || desc->object_id == 0) {
        return BHARAT_MSG_ERR_INVALID_CAP;
    }

    // A delegated token without a nonce is unsafe to import
    if (desc->nonce == 0) {
        return BHARAT_MSG_ERR_UNAUTHORIZED;
    }

    return BHARAT_MSG_OK;
}

void bharat_capwire_attenuate(bharat_capwire_desc_t* desc, uint32_t allowed_rights) {
    if (desc) {
        // Strip any bits that are not present in allowed_rights
        desc->rights_mask &= allowed_rights;
    }
}
