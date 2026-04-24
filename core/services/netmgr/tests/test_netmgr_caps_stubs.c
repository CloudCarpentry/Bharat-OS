#include <bharat/ipc/ipc.h>
#include <bharat/uapi/ipc/status.h>

int32_t bharat_ipc_call(bharat_ipc_endpoint_t endpoint, const bharat_ipc_msg_header_t *req_header, const void *req_payload,
                        bharat_ipc_msg_header_t *rep_header, void *rep_payload_buf, uint32_t rep_max_size) {
    return BHARAT_IPC_STATUS_OK;
}
