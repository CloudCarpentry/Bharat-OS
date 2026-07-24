#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <bharat/service/service_runtime.h>
#include <bharat/uapi/service_status.h>

// Mock implementation of service-specific logic
bharat_status_t bh_service_handle_msg(bh_service_ctx_t *ctx, const bh_msg_t *msg) {
    (void)ctx;
    if (msg->header.opcode == 1) return BHARAT_STATUS_OK;
    return BHARAT_STATUS_ERR_UNSUPPORTED;
}

// Mock IPC
int32_t bharat_ipc_send(bharat_ipc_endpoint_t endpoint, const bharat_ipc_msg_header_t *header, const void *payload) { (void)endpoint; (void)header; (void)payload; return 0; }
int32_t bharat_ipc_recv(bharat_ipc_endpoint_t endpoint, bharat_ipc_msg_header_t *header, void *payload_buf, uint32_t max_size) {
    (void)endpoint; (void)max_size;
    header->opcode = 1;
    header->payload_size = 0;
    return 0;
}

void test_runtime_loop(void) {
    bh_service_start_info_t info = {
        .service_id = 1,
        .service_name = "test_service",
        .endpoint = 0x1000
    };

    // We can't easily test bh_service_main because it loops forever
    // but we can test poll_once
    bh_service_ctx_t ctx = {
        .service_id = 1,
        .endpoint = 0x1000
    };

    assert(bh_service_poll_once(&ctx) == BHARAT_STATUS_OK);

    printf("test_runtime_loop passed\n");
}

int main(void) {
    test_runtime_loop();
    return 0;
}
