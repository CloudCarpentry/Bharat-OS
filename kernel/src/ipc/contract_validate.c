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

    return BHARAT_IPC_STATUS_OK;
}
