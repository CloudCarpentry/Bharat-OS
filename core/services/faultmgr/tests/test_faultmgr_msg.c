#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <bharat/service/service_runtime.h>
#include <bharat/uapi/services/service_ids.h>
#include <bharat/uapi/faultmgr/contract.h>
#include "../state.h"

// Forward declaration from main.c
bharat_status_t bh_service_handle_msg(bh_service_ctx_t *ctx, const bh_msg_t *msg);

// Mock clock for host testing
int bh_time_get(uint32_t clock_id, uint64_t* out_time) {
    if (out_time) *out_time = 1000;
    return 0;
}

// Mock IPC send
int32_t bharat_ipc_send(bharat_ipc_endpoint_t endpoint, const bharat_ipc_msg_header_t *header, const void *payload) {
    return 0;
}

static void test_msg_handler(void) {
    printf("Running test_msg_handler...\n");
    faultmgr_state_init();

    bh_service_ctx_t ctx = {
        .service_id = BHARAT_SERVICE_FAULTMGR,
        .incarnation_id = 1,
        .endpoint = 0x1000,
        .should_exit = false
    };

    // Invalid args
    assert(bh_service_handle_msg(NULL, NULL) == BHARAT_STATUS_ERR_INVALID_ARG);

    // Invalid Opcode
    bh_msg_t msg = {
        .header = { .opcode = 999 },
        .payload = NULL,
        .payload_size = 0
    };
    assert(bh_service_handle_msg(&ctx, &msg) == BHARAT_STATUS_ERR_OPCODE);

    // Invalid Length
    msg.header.opcode = FAULTMGR_OP_REPORT_FAULT;
    msg.payload_size = 4; // Too small
    assert(bh_service_handle_msg(&ctx, &msg) == BHARAT_STATUS_ERR_LENGTH);

    // Valid Message
    faultmgr_req_report_fault_t req = {
        .domain_id = 5,
        .source_id = 50,
        .category = FAULT_CAT_CRASH,
        .severity = FAULT_SEV_ERROR,
        .incarnation_id = 1
    };
    msg.payload = &req;
    msg.payload_size = sizeof(req);

    assert(bh_service_handle_msg(&ctx, &msg) == BHARAT_STATUS_OK);

    const fault_record_t* rec = faultmgr_get_record(5);
    assert(rec != NULL);
    assert(rec->failure_count == 1);

    printf("Passed.\n");
}

int main(void) {
    test_msg_handler();
    printf("All faultmgr message handler host tests passed.\n");
    return 0;
}
