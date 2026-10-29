#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "vm_manager.h"
#include <bharat/uapi/ipc/status.h>

/* Deliberately bypass libipc's sanitization to test the service boundary itself.
 * This is a denial-path unit test, not authenticated kernel transport evidence. */
static bharat_ipc_msg_header_t incoming;
static unsigned sends;
static int32_t receive_status;

int32_t bharat_ipc_recv(bharat_ipc_endpoint_t endpoint,
                       bharat_ipc_msg_header_t *header, void *payload,
                       uint32_t capacity) {
    (void)endpoint;
    if (receive_status != 0) return receive_status;
    assert(capacity >= incoming.payload_size);
    *header = incoming;
    memset(payload, 0xff, incoming.payload_size);
    return 0;
}

int32_t bharat_ipc_send(bharat_ipc_endpoint_t endpoint,
                       const bharat_ipc_msg_header_t *header, const void *payload) {
    (void)endpoint; (void)header; (void)payload;
    ++sends;
    return 0;
}

int main(void) {
    vm_manager_init();
    incoming = (bharat_ipc_msg_header_t){
        .header_version = BHARAT_IPC_HEADER_VERSION_V1,
        .service_id = VM_MANAGER_SERVICE_ID,
        .interface_version = BH_VM_INTERFACE_VERSION_V1,
        .opcode = BH_VM_OP_CREATE_SPACE_V1,
        .payload_size = sizeof(bh_vm_create_space_request_v1_t),
        .capability_transfer = UINT64_MAX,
        .reply_endpoint = UINT64_MAX,
    };
    /* Guessed handles, even ones made equal, cannot become authority/reply. */
    for (unsigned i = 0; i < 10000; ++i) {
        incoming.capability_transfer = i;
        incoming.reply_endpoint = i;
        assert(bh_vm_manager_poll_once(1) == BHARAT_IPC_STATUS_ERR_PERM);
    }
    assert(bh_vm_get_active_spaces_count() == 0);
    assert(bh_vm_get_active_regions_count() == 0);
    assert(sends == 0);

    ++incoming.payload_size;
    assert(bh_vm_manager_poll_once(1) == BHARAT_IPC_STATUS_ERR_LENGTH);
    incoming.interface_version = 2;
    assert(bh_vm_manager_poll_once(1) == BHARAT_IPC_STATUS_ERR_VERSION);
    incoming.interface_version = 0;
    incoming.opcode = VM_OP_MAP;
    incoming.payload_size = sizeof(vm_req_map_t);
    assert(bh_vm_manager_poll_once(1) == BHARAT_IPC_STATUS_ERR_PERM);
    vm_req_query_t query = {.region_id = 1};
    vm_resp_query_t result = {0};
    assert(vm_manager_handle_query(&query, &result) == BHARAT_IPC_STATUS_ERR_NOT_FOUND);

    incoming.flags = BHARAT_IPC_FLAG_REPLY;
    assert(bh_vm_manager_poll_once(1) == BHARAT_IPC_STATUS_ERR_INVALID);
    incoming.flags = BHARAT_IPC_FLAG_CAP_TRANSFER;
    assert(bh_vm_manager_poll_once(1) == BHARAT_IPC_STATUS_ERR_INVALID);
    incoming.flags = 0;
    incoming.service_id = 0;
    assert(bh_vm_manager_poll_once(1) == BHARAT_IPC_STATUS_ERR_INVALID);
    receive_status = BHARAT_IPC_STATUS_ERR_ENDPOINT;
    assert(bh_vm_manager_poll_once(1) == receive_status);
    assert(sends == 0);
    puts("VM wire boundary denial tests passed (no authenticated transport claimed)");
    return 0;
}
