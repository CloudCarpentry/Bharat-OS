#ifndef BHARAT_IPC_STATUS_H
#define BHARAT_IPC_STATUS_H

#include <bharat/uapi/ipc/status.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file ipc_status.h
 * @brief Human-readable status mapping for IPC error codes.
 */

/**
 * Returns a human-readable string representation of a bharat_status_t code.
 * @param status The status code to map.
 * @return A constant string describing the status.
 */
const char *bharat_ipc_status_to_string(bharat_status_t status);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_IPC_STATUS_H
