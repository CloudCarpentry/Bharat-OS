<<<<<<< HEAD
#ifndef BHARAT_KERNEL_IPC_CONTRACT_VALIDATE_H
#define BHARAT_KERNEL_IPC_CONTRACT_VALIDATE_H

#include <stdint.h>
#include "uapi/bharat/ipc/contract.h"
#include "uapi/bharat/ipc/status.h"

typedef struct bharat_ipc_contract_validate_rules {
    uint32_t min_interface_version;
    uint32_t max_interface_version;
    uint32_t min_opcode;
    uint32_t max_opcode;
    uint32_t min_payload_size;
    uint32_t max_payload_size;
    uint32_t allowed_flags_mask;
} bharat_ipc_contract_validate_rules_t;

bharat_ipc_status_t bharat_ipc_contract_validate_header(
    const bharat_ipc_contract_header_t *hdr,
    uint32_t header_bytes,
    uint32_t total_message_bytes,
    const bharat_ipc_contract_validate_rules_t *rules);

#endif /* BHARAT_KERNEL_IPC_CONTRACT_VALIDATE_H */
=======
#ifndef BHARAT_IPC_CONTRACT_VALIDATE_H
#define BHARAT_IPC_CONTRACT_VALIDATE_H

#include <uapi/bharat/ipc/contract.h>
#include <uapi/bharat/ipc/status.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file contract_validate.h
 * @brief Helper functions to validate IPC contract headers.
 */

/**
 * Validates the core structure of an IPC contract header.
 * @param hdr The header to validate.
 * @param expected_version The interface version expected by the handler.
 * @param min_opcode Minimum valid opcode.
 * @param max_opcode Maximum valid opcode.
 * @param expected_payload_size Optional explicit payload size to check against (0 to skip size check if variable).
 * @return BHARAT_IPC_STATUS_OK on success, or a canonical IPC status error code.
 */
int bharat_ipc_contract_validate(const bharat_ipc_contract_header_t *hdr,
                                 uint32_t expected_version,
                                 uint32_t min_opcode,
                                 uint32_t max_opcode,
                                 uint32_t expected_payload_size);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_IPC_CONTRACT_VALIDATE_H
>>>>>>> 037b676 (WIP: IPC contract baseline)
