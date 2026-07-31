#include <stdint.h>
#include <stddef.h>
#include <bharat/ipc/ipc.h>
#include <bharat/cap/cap_authz.h>

int32_t bharat_service_dispatch_authorize(
    uint32_t service_id,
    uint32_t opcode,
    const bharat_service_authz_desc_t *authz_descs,
    uint32_t count,
    bharat_cap_handle_t caller_cap,
    uint64_t target_object_id)
{
    (void)service_id; (void)opcode; (void)authz_descs; (void)count; (void)caller_cap; (void)target_object_id;
    return 0; // success
}

int32_t bharat_ipc_recv(bharat_ipc_endpoint_t endpoint, bharat_ipc_msg_header_t *header, void *payload, uint32_t max_payload_size) {
    (void)endpoint; (void)header; (void)payload; (void)max_payload_size;
    return -1; // block
}

int32_t bharat_ipc_send(bharat_ipc_endpoint_t endpoint, const bharat_ipc_msg_header_t *header, const void *payload) {
    (void)endpoint; (void)header; (void)payload;
    return 0;
}
