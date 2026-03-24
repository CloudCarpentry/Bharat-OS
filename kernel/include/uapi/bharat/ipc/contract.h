#ifndef BHARAT_UAPI_IPC_CONTRACT_H
#define BHARAT_UAPI_IPC_CONTRACT_H

#include <stdint.h>

/*
 * Canonical service IPC contract envelope.
 * This is a service/object contract header (L2/L3), not a raw transport frame.
 */

typedef uint64_t bharat_ipc_cap_token_t;

enum {
    BHARAT_IPC_CONTRACT_FLAG_NONE         = 0u,
    BHARAT_IPC_CONTRACT_FLAG_CAP_TRANSFER = 1u << 0,
    BHARAT_IPC_CONTRACT_FLAG_REPLY_CAP    = 1u << 1,
    BHARAT_IPC_CONTRACT_FLAG_ONEWAY       = 1u << 2,
};

typedef struct bharat_ipc_contract_header {
    uint32_t service_id;
    uint32_t interface_id;
    uint32_t interface_version;
    uint32_t opcode;
    uint32_t flags;
    uint32_t payload_size;
    uint32_t message_id;
    uint32_t reserved0;
    bharat_ipc_cap_token_t capability_transfer;
    bharat_ipc_cap_token_t reply_endpoint;
} bharat_ipc_contract_header_t;

#endif /* BHARAT_UAPI_IPC_CONTRACT_H */
