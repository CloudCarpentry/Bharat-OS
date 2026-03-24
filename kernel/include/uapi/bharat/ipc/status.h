#ifndef BHARAT_UAPI_IPC_STATUS_H
#define BHARAT_UAPI_IPC_STATUS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file status.h
 * @brief Canonical IPC contract-level statuses.
 */

typedef enum {
    BHARAT_IPC_STATUS_OK = 0,
    BHARAT_IPC_STATUS_ERR_DECODE = -1001,
    BHARAT_IPC_STATUS_ERR_VERSION = -1002,
    BHARAT_IPC_STATUS_ERR_OPCODE = -1003,
    BHARAT_IPC_STATUS_ERR_PERM = -1004,
    BHARAT_IPC_STATUS_ERR_NOT_FOUND = -1005,
    BHARAT_IPC_STATUS_ERR_UNSUPPORTED = -1006,
    BHARAT_IPC_STATUS_ERR_INTERNAL = -1007,
} bharat_ipc_contract_status_t;

#ifdef __cplusplus
}
#endif

#endif // BHARAT_UAPI_IPC_STATUS_H
