#ifndef BHARAT_IDL_OOL_H
#define BHARAT_IDL_OOL_H

#include "bharat/msg/types.h"
#include "bharat/msg/errors.h"
#include "bharat/msg/payload.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// OOL Descriptor Types
// ============================================================================

#define BHARAT_OOL_TYPE_SHMEM  0
#define BHARAT_OOL_TYPE_DMA    1
#define BHARAT_OOL_TYPE_BLOB   2
#define BHARAT_OOL_TYPE_PACKET 3

// ============================================================================
// OOL Descriptor Flags
// ============================================================================

#define BHARAT_OOL_FLAG_READ_ONLY   (1U << 0)
#define BHARAT_OOL_FLAG_READ_WRITE  (1U << 1)
#define BHARAT_OOL_FLAG_VOLATILE    (1U << 2)

// ============================================================================
// OOL Descriptor (Normalized In-Memory)
// ============================================================================
// Represents a reference to an Out-Of-Line memory block or region, typically
// trailing the capwire array at the end of a message payload.
// ============================================================================

typedef struct {
    uint8_t  desc_type; // BHARAT_OOL_TYPE_*
    uint8_t  flags;     // BHARAT_OOL_FLAG_*
    uint64_t region_id; // ID of the referenced object/memory region
    uint64_t offset;    // Starting offset within the region
    uint64_t length;    // Total length of the referenced chunk
} bharat_ool_desc_t;

// ============================================================================
// OOL Codec API
// ============================================================================

/**
 * @brief Encode an OOL descriptor into the payload builder.
 * @param b Builder cursor
 * @param desc Descriptor to encode
 * @return BHARAT_MSG_OK or negative error
 */
int bharat_ool_encode(bharat_msg_builder_t* b, const bharat_ool_desc_t* desc);

/**
 * @brief Decode an OOL descriptor from the payload reader.
 * @param r Reader cursor
 * @param desc Output descriptor
 * @return BHARAT_MSG_OK or negative error
 */
int bharat_ool_decode(bharat_msg_reader_t* r, bharat_ool_desc_t* desc);

// ============================================================================
// Validation Hook
// ============================================================================

/**
 * @brief Validate the logical structure of the OOL descriptor
 * Reject invalid types, unmapped flags, or zero-length payload claims
 */
int bharat_ool_validate(const bharat_ool_desc_t* desc);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_IDL_OOL_H
