#ifndef BHARAT_IDL_CAPWIRE_H
#define BHARAT_IDL_CAPWIRE_H

#include "bharat/msg/types.h"
#include "bharat/msg/errors.h"
#include "bharat/msg/payload.h"

// Do not include kernel-internal headers directly here to remain transport/app agnostic.
// We strictly define the wire format for capabilities.

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Transfer Modes
// ============================================================================

#define BHARAT_CAP_XFER_COPY                0
#define BHARAT_CAP_XFER_MOVE                1
#define BHARAT_CAP_XFER_BORROW              2
#define BHARAT_CAP_XFER_DELEGATE_ATTENUATED 3

// ============================================================================
// Capwire Descriptor (Normalized In-Memory)
// ============================================================================
// Represents the remote capability token transmitted across the wire.
// ============================================================================

typedef struct {
    uint8_t  cap_type;      // e.g. CAP_OBJ_ENDPOINT, CAP_OBJ_MEMORY, etc.
    uint8_t  transfer_mode; // BHARAT_CAP_XFER_*
    uint32_t rights_mask;   // Access rights authorized
    uint32_t origin_node;   // ID of the node originating this cap
    uint32_t issuer_id;     // ID of the issuer domain
    uint64_t object_id;     // ID of the underlying resource/object
    uint64_t nonce;         // Cryptographic random nonce for replay/revocation checks
    uint32_t generation;    // Helps track versioning or revoke status
} bharat_capwire_desc_t;

// ============================================================================
// Capwire Codec API
// ============================================================================

/**
 * @brief Encode a capability descriptor into the payload builder.
 * @param b Builder cursor
 * @param desc Descriptor to encode
 * @return BHARAT_MSG_OK or negative error
 */
int bharat_capwire_encode(bharat_msg_builder_t* b, const bharat_capwire_desc_t* desc);

/**
 * @brief Decode a capability descriptor from the payload reader.
 * @param r Reader cursor
 * @param desc Output descriptor
 * @return BHARAT_MSG_OK or negative error
 */
int bharat_capwire_decode(bharat_msg_reader_t* r, bharat_capwire_desc_t* desc);

// ============================================================================
// Internal Validation & Proxy Hooks (Implemented in capwire library)
// ============================================================================

/**
 * @brief Validate the semantics of a wire descriptor.
 * Checks for invalid transfer modes or malformed IDs before proxying.
 */
int bharat_capwire_validate(const bharat_capwire_desc_t* desc);

/**
 * @brief Attenuate the rights mask.
 * Drops bits not present in the allowed mask.
 */
void bharat_capwire_attenuate(bharat_capwire_desc_t* desc, uint32_t allowed_rights);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_IDL_CAPWIRE_H
