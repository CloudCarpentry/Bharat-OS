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

    return BHARAT_IPC_STATUS_OK;
}