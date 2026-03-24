#ifndef BHARAT_UAPI_IPC_MANIFEST_H
#define BHARAT_UAPI_IPC_MANIFEST_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file manifest.h
 * @brief Common static manifest types for BIDL and IPC service descriptions.
 */

// An operation defined by an interface manifest
typedef struct {
    uint32_t opcode;
    const char *name;
    size_t min_request_size;
    size_t max_request_size;
    size_t response_size;
    uint32_t required_rights;
} bharat_ipc_op_manifest_t;

// An interface manifest published by a service
typedef struct {
    const char *service_name;
    const char *interface_name;
    uint32_t interface_version;
    uint32_t transport_kind;
    uint32_t operation_count;
    const bharat_ipc_op_manifest_t *operations;
} bharat_ipc_service_manifest_t;

#ifdef __cplusplus
}
#endif

#endif // BHARAT_UAPI_IPC_MANIFEST_H
