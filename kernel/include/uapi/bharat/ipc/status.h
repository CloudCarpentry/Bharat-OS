#ifndef BHARAT_UAPI_IPC_STATUS_H
#define BHARAT_UAPI_IPC_STATUS_H

<<<<<<< HEAD
#include <stdint.h>

/*
 * Canonical service-contract level status codes.
 * These are transport/contract semantics and should remain distinct from
 * service-specific business status codes.
 */
typedef int32_t bharat_ipc_status_t;

#define BHARAT_IPC_STATUS_OK               ((bharat_ipc_status_t)0)
#define BHARAT_IPC_STATUS_ERR_DECODE       ((bharat_ipc_status_t)-1)
#define BHARAT_IPC_STATUS_ERR_VERSION      ((bharat_ipc_status_t)-2)
#define BHARAT_IPC_STATUS_ERR_OPCODE       ((bharat_ipc_status_t)-3)
#define BHARAT_IPC_STATUS_ERR_PERM         ((bharat_ipc_status_t)-4)
#define BHARAT_IPC_STATUS_ERR_NOT_FOUND    ((bharat_ipc_status_t)-5)
#define BHARAT_IPC_STATUS_ERR_UNSUPPORTED  ((bharat_ipc_status_t)-6)
#define BHARAT_IPC_STATUS_ERR_INTERNAL     ((bharat_ipc_status_t)-7)

/* Optional transport/decode-oriented extensions used by validators/runtime. */
#define BHARAT_IPC_STATUS_ERR_TRUNCATED    ((bharat_ipc_status_t)-8)
#define BHARAT_IPC_STATUS_ERR_LENGTH       ((bharat_ipc_status_t)-9)
#define BHARAT_IPC_STATUS_ERR_FLAGS        ((bharat_ipc_status_t)-10)

#endif /* BHARAT_UAPI_IPC_STATUS_H */
=======
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
>>>>>>> 037b676 (WIP: IPC contract baseline)
