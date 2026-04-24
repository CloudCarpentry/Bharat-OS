#include "bharat/idl/ool.h"

int bharat_ool_validate(const bharat_ool_desc_t* desc) {
    if (!desc) {
        return BHARAT_MSG_ERR_MALFORMED_PAYLOAD;
    }

    if (desc->desc_type > BHARAT_OOL_TYPE_PACKET) {
        return BHARAT_MSG_ERR_MALFORMED_PAYLOAD;
    }

    if (desc->length == 0) {
        // Zero-length OOL descriptor usually indicates an implementation bug,
        // there's no reason to send an empty out-of-line mapping
        return BHARAT_MSG_ERR_MALFORMED_PAYLOAD;
    }

    // region_id must exist
    if (desc->region_id == 0) {
        return BHARAT_MSG_ERR_MALFORMED_PAYLOAD;
    }

    return BHARAT_MSG_OK;
}
