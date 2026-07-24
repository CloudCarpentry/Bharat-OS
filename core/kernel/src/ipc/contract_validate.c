#include <ipc/contract_validate.h>
#include <stddef.h>

/**
 * Validates the core structure of an IPC contract header against a set of rules.
 */
int bharat_ipc_contract_validate_ex(const bharat_ipc_contract_header_t *hdr,
                                    const ipc_contract_rules_t *rules)
{
    if (hdr == NULL || rules == NULL) {
        return BHARAT_IPC_STATUS_ERR_DECODE;
    }

    /* Validate header version */
    if (hdr->header_version != BHARAT_IPC_HEADER_VERSION_V1) {
        return BHARAT_IPC_STATUS_ERR_VERSION;
    }

    /* Validate interface version */
    if (hdr->interface_version != rules->expected_version) {
        return BHARAT_IPC_STATUS_ERR_VERSION;
    }

    /* Validate opcode range */
    if (hdr->opcode < rules->min_opcode || hdr->opcode > rules->max_opcode) {
        return BHARAT_IPC_STATUS_ERR_OPCODE;
    }

    /* Validate payload size */
    if (rules->allow_variable_payload) {
        if (rules->max_payload_size > 0 && hdr->payload_size > rules->max_payload_size) {
            return BHARAT_IPC_STATUS_ERR_LENGTH;
        }
    } else {
        if (rules->max_payload_size > 0 && hdr->payload_size != rules->max_payload_size) {
            return BHARAT_IPC_STATUS_ERR_LENGTH;
        }
    }

    /* Validate flags */
    if ((hdr->flags & rules->required_flags) != rules->required_flags) {
        return BHARAT_IPC_STATUS_ERR_FLAGS;
    }

    if ((hdr->flags & ~rules->allowed_flags) != 0) {
        return BHARAT_IPC_STATUS_ERR_FLAGS;
    }

    /* Reserved flags check: Reject any bits outside the defined V1 flags */
    if (hdr->flags & ~BHARAT_IPC_FLAGS_ALL_V1) {
        return BHARAT_IPC_STATUS_ERR_FLAGS;
    }

    /* Payload alignment check */
    if (rules->payload_alignment > 1) {
        if (hdr->payload_size % rules->payload_alignment != 0) {
            return BHARAT_IPC_STATUS_ERR_DECODE;
        }
    }

    return BHARAT_IPC_STATUS_OK;
}

/**
 * Validates the core structure of an IPC contract header. (Legacy wrapper)
 */
int bharat_ipc_contract_validate(const bharat_ipc_contract_header_t *hdr,
                                 uint32_t expected_version,
                                 uint32_t min_opcode,
                                 uint32_t max_opcode,
                                 uint32_t expected_payload_size)
{
    ipc_contract_rules_t rules = {
        .expected_version = expected_version,
        .min_opcode = min_opcode,
        .max_opcode = max_opcode,
        .max_payload_size = expected_payload_size,
        .required_flags = 0,
        .allowed_flags = BHARAT_IPC_FLAGS_ALL_V1,
        .allow_variable_payload = (expected_payload_size == 0),
        .payload_alignment = 0,
    };

    return bharat_ipc_contract_validate_ex(hdr, &rules);
}
