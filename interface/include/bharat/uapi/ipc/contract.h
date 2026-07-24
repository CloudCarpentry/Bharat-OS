#ifndef BHARAT_UAPI_IPC_CONTRACT_H
#define BHARAT_UAPI_IPC_CONTRACT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file contract.h
 * @brief Canonical service IPC contract envelope.
 */

/* Service IPC Contract Header */
typedef struct {
    uint32_t header_version;
    uint32_t service_id;
    uint32_t interface_id;
    uint32_t interface_version;
    uint32_t opcode;
    uint32_t flags;
    uint32_t payload_size;
    int32_t status;
    uint64_t message_id;
    uint64_t capability_transfer; // Optional capability transfer token
    uint64_t reply_endpoint;      // Optional reply capability/endpoint
} bharat_ipc_contract_header_t;

enum {
    BHARAT_IPC_HEADER_VERSION_V1 = 1u,
};

enum {
    BHARAT_IPC_FLAG_REPLY          = (1u << 0),
    BHARAT_IPC_FLAG_NONBLOCK       = (1u << 1),
    BHARAT_IPC_FLAG_CAP_TRANSFER   = (1u << 2),
};

#define BHARAT_IPC_FLAGS_ALL_V1 (BHARAT_IPC_FLAG_REPLY | \
                                 BHARAT_IPC_FLAG_NONBLOCK | \
                                 BHARAT_IPC_FLAG_CAP_TRANSFER)

#ifdef __cplusplus
}
#endif

#endif // BHARAT_UAPI_IPC_CONTRACT_H
