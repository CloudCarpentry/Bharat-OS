<<<<<<< HEAD
#include "../../include/ipc/contract_validate.h"

#include <stddef.h>

bharat_ipc_status_t bharat_ipc_contract_validate_header(
    const bharat_ipc_contract_header_t *hdr,
    uint32_t header_bytes,
    uint32_t total_message_bytes,
    const bharat_ipc_contract_validate_rules_t *rules) {
    if (!hdr || !rules) {
        return BHARAT_IPC_STATUS_ERR_DECODE;
    }

    if (header_bytes < sizeof(*hdr)) {
        return BHARAT_IPC_STATUS_ERR_TRUNCATED;
    }

    if (total_message_bytes < header_bytes) {
        return BHARAT_IPC_STATUS_ERR_TRUNCATED;
    }

    if (hdr->payload_size < rules->min_payload_size ||
        hdr->payload_size > rules->max_payload_size) {
        return BHARAT_IPC_STATUS_ERR_LENGTH;
    }

    if (hdr->interface_version < rules->min_interface_version ||
        hdr->interface_version > rules->max_interface_version) {
        return BHARAT_IPC_STATUS_ERR_VERSION;
    }

    if (hdr->opcode < rules->min_opcode || hdr->opcode > rules->max_opcode) {
        return BHARAT_IPC_STATUS_ERR_OPCODE;
    }

    if ((hdr->flags & ~rules->allowed_flags_mask) != 0u) {
        return BHARAT_IPC_STATUS_ERR_FLAGS;
    }

    if (hdr->payload_size > (total_message_bytes - header_bytes)) {
        return BHARAT_IPC_STATUS_ERR_TRUNCATED;
    }
=======
#include <ipc/contract_validate.h>
#include <stddef.h>

/**
 * Validates the core structure of an IPC contract header.
 */
int bharat_ipc_contract_validate(const bharat_ipc_contract_header_t *hdr,
                                 uint32_t expected_version,
                                 uint32_t min_opcode,
                                 uint32_t max_opcode,
                                 uint32_t expected_payload_size)
{
    if (hdr == NULL) {
        return BHARAT_IPC_STATUS_ERR_DECODE;
    }

    /* Validate version */
    if (hdr->interface_version != expected_version) {
        return BHARAT_IPC_STATUS_ERR_VERSION;
    }

    /* Validate opcode range */
    if (hdr->opcode < min_opcode || hdr->opcode > max_opcode) {
        return BHARAT_IPC_STATUS_ERR_OPCODE;
    }

    /* Validate payload size (if a specific size is expected)
     * For ops that allow a variable size payload within bounds,
     * the caller can set expected_payload_size=0 and do its own check.
     */
    if (expected_payload_size > 0 && hdr->payload_size != expected_payload_size) {
        return BHARAT_IPC_STATUS_ERR_DECODE;
    }

    /* In the future, flags checks might be added here */
>>>>>>> 037b676 (WIP: IPC contract baseline)

    return BHARAT_IPC_STATUS_OK;
}
