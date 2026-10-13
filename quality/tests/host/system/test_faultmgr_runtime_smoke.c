#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <bharat/uapi/services/service_ids.h>
#include <bharat/uapi/faultmgr/contract.h>
#include <bharat/service/service_runtime.h>

extern void init_faultmgr(void);
extern bharat_status_t bh_service_handle_msg(bh_service_ctx_t *ctx, const bh_msg_t *msg);

// Mock IPC send
int32_t bharat_ipc_send(bharat_ipc_endpoint_t endpoint, const bharat_ipc_msg_header_t *header, const void *payload) {
    return 0;
}
// Mock clock for host testing
int bh_time_get(uint32_t clock_id, uint64_t* out_time) {
    if (out_time) *out_time = 1000;
    return 0;
}

static void test_faultmgr_runtime_smoke(void) {
    printf("Running test_faultmgr_runtime_smoke...\n");
    init_faultmgr();

    bh_service_ctx_t ctx = {
        .service_id = BHARAT_SERVICE_FAULTMGR,
        .incarnation_id = 1,
        .endpoint = 0x1000, // Mock endpoint
        .should_exit = false
    };

    faultmgr_req_report_fault_t req = {
        .domain_id = 99,
        .source_id = 990,
        .category = FAULT_CAT_CRASH,
        .severity = FAULT_SEV_ERROR,
        .incarnation_id = 1
    };

    bh_msg_t msg = {
        .header = {
            .opcode = FAULTMGR_OP_REPORT_FAULT,
            .flags = BHARAT_IPC_FLAG_REPLY,
            .reply_endpoint = 0x2000 // Mock reply endpoint
        },
        .payload = &req,
        .payload_size = sizeof(req)
    };

    // Process a valid event
    bharat_status_t status = bh_service_handle_msg(&ctx, &msg);
    assert(status == BHARAT_STATUS_OK);

    // Verify it stays alive
    assert(ctx.should_exit == false);

    printf("test_faultmgr_runtime_smoke Passed.\n");
}

int main(void) {
    test_faultmgr_runtime_smoke();
    return 0;
}
