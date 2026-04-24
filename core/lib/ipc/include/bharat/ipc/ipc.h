#ifndef BHARAT_IPC_H
#define BHARAT_IPC_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/cap/cap.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file ipc.h
 * @brief Higher-level IPC wrapper above raw syscall/URPC.
 */

/* IPC Endpoint Handle Type */
typedef bharat_cap_handle_t bharat_ipc_endpoint_t;

/* Include UAPI contract and status headers for service-level IPC */
#include <bharat/uapi/ipc/contract.h>
#include <bharat/uapi/ipc/status.h>

/* IPC Message Header Struct - compatibility alias */
typedef bharat_ipc_contract_header_t bharat_ipc_msg_header_t;

/* IPC Send Request Stub */
int32_t bharat_ipc_send(bharat_ipc_endpoint_t endpoint, const bharat_ipc_msg_header_t *header, const void *payload);

/* IPC Receive Request Stub */
int32_t bharat_ipc_recv(bharat_ipc_endpoint_t endpoint, bharat_ipc_msg_header_t *header, void *payload_buf, uint32_t max_size);

/* IPC Request-Reply Stub */
int32_t bharat_ipc_call(bharat_ipc_endpoint_t endpoint, const bharat_ipc_msg_header_t *req_header, const void *req_payload,
                        bharat_ipc_msg_header_t *rep_header, void *rep_payload_buf, uint32_t rep_max_size);

/* Extended timeout-aware APIs (ticks, UINT64_MAX means wait forever). */
int32_t bharat_ipc_send_ex(bharat_ipc_endpoint_t endpoint,
                           const bharat_ipc_msg_header_t *header,
                           const void *payload,
                           uint64_t timeout_ticks);
int32_t bharat_ipc_recv_ex(bharat_ipc_endpoint_t endpoint,
                           bharat_ipc_msg_header_t *header,
                           void *payload_buf,
                           uint32_t max_size,
                           uint64_t timeout_ticks);
int32_t bharat_ipc_call_ex(bharat_ipc_endpoint_t endpoint,
                           const bharat_ipc_msg_header_t *req_header,
                           const void *req_payload,
                           bharat_ipc_msg_header_t *rep_header,
                           void *rep_payload_buf,
                           uint32_t rep_max_size,
                           uint64_t timeout_ticks);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_IPC_H
