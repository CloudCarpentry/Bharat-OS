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
