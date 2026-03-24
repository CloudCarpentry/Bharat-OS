#ifndef BHARAT_UAPI_IPC_STATUS_H
#define BHARAT_UAPI_IPC_STATUS_H

#include <bharat/uapi/service_status.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file status.h
 * @brief Canonical IPC contract-level statuses.
 */

/* Backward-compatible aliases for older include paths. */
typedef bharat_status_t bharat_ipc_contract_status_t;

#define BHARAT_IPC_STATUS_OK              BHARAT_STATUS_OK
#define BHARAT_IPC_STATUS_ERR_DECODE      BHARAT_STATUS_ERR_DECODE
#define BHARAT_IPC_STATUS_ERR_VERSION     BHARAT_STATUS_ERR_VERSION
#define BHARAT_IPC_STATUS_ERR_OPCODE      BHARAT_STATUS_ERR_OPCODE
#define BHARAT_IPC_STATUS_ERR_PERM        BHARAT_STATUS_ERR_PERMISSION
#define BHARAT_IPC_STATUS_ERR_NOT_FOUND   BHARAT_STATUS_ERR_NOT_FOUND
#define BHARAT_IPC_STATUS_ERR_UNSUPPORTED BHARAT_STATUS_ERR_UNSUPPORTED
#define BHARAT_IPC_STATUS_ERR_INTERNAL    BHARAT_STATUS_ERR_INTERNAL
#define BHARAT_IPC_STATUS_ERR_TRUNCATED   BHARAT_STATUS_ERR_TRUNCATED
#define BHARAT_IPC_STATUS_ERR_LENGTH      BHARAT_STATUS_ERR_LENGTH
#define BHARAT_IPC_STATUS_ERR_FLAGS       BHARAT_STATUS_ERR_FLAGS

#ifdef __cplusplus
}
#endif

#endif // BHARAT_UAPI_IPC_STATUS_H
