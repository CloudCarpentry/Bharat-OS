#ifndef BHARAT_MSG_VALIDATE_H
#define BHARAT_MSG_VALIDATE_H

#include "bharat/msg/wire.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Strictly validate the logical correctness of a decoded header.
 *
 * Ensures length boundaries (total_len >= header_len),
 * valid flag combinations, and sane field sizes prior to service dispatch.
 * Does NOT check CRC (that is usually done during decode/transport stage).
 *
 * @param hdr The parsed internal header.
 * @param max_mtu Maximum valid total size on this node/transport (or 0 for unconstrained).
 * @return BHARAT_MSG_OK if valid, negative BHARAT_MSG_ERR_* code otherwise.
 */
int bharat_msg_header_validate(const bharat_msg_header_t* hdr, size_t max_mtu);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_MSG_VALIDATE_H
