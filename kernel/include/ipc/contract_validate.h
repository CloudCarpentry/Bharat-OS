#ifndef BHARAT_IPC_CONTRACT_VALIDATE_H
#define BHARAT_IPC_CONTRACT_VALIDATE_H

#include <bharat/uapi/ipc/contract.h>
#include <bharat/uapi/ipc/status.h>
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
