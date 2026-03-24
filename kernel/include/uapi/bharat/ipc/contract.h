#ifndef BHARAT_UAPI_IPC_CONTRACT_H
#define BHARAT_UAPI_IPC_CONTRACT_H

#include <stdint.h>

<<<<<<< HEAD
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
=======
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file contract.h
 * @brief Canonical service IPC contract envelope.
 */

/* Service IPC Contract Header */
typedef struct {
>>>>>>> 037b676 (WIP: IPC contract baseline)
    uint32_t service_id;
    uint32_t interface_id;
    uint32_t interface_version;
    uint32_t opcode;
    uint32_t flags;
    uint32_t payload_size;
<<<<<<< HEAD
    uint32_t message_id;
    uint32_t reserved0;
    bharat_ipc_cap_token_t capability_transfer;
    bharat_ipc_cap_token_t reply_endpoint;
} bharat_ipc_contract_header_t;

#endif /* BHARAT_UAPI_IPC_CONTRACT_H */
=======
    uint64_t message_id;
    uint64_t capability_transfer; // Optional capability transfer token
    uint64_t reply_endpoint;      // Optional reply capability/endpoint
} bharat_ipc_contract_header_t;

#ifdef __cplusplus
}
#endif

#endif // BHARAT_UAPI_IPC_CONTRACT_H
>>>>>>> 037b676 (WIP: IPC contract baseline)
