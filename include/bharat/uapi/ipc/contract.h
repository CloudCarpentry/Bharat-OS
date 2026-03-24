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
    uint32_t service_id;
    uint32_t interface_id;
    uint32_t interface_version;
    uint32_t opcode;
    uint32_t flags;
    uint32_t payload_size;
    uint64_t message_id;
    uint64_t capability_transfer; // Optional capability transfer token
    uint64_t reply_endpoint;      // Optional reply capability/endpoint
} bharat_ipc_contract_header_t;

#ifdef __cplusplus
}
#endif

#endif // BHARAT_UAPI_IPC_CONTRACT_H
