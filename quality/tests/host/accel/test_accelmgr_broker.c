#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "accelmgr_broker.h"
#include <bharat/cap/cap_validate.h>
#include <bharat/uapi/capability/rights.h>

// Capability validation mock backend
static bharat_cap_status_t mock_cap_validate(
    bharat_cap_handle_t handle,
    bharat_cap_object_type_t expected_object_type,
    uint64_t expected_object_id,
    uint64_t required_rights,
    const bharat_cap_scope_t *required_scope,
    bharat_cap_validation_result_t *out_result)
{
    (void)expected_object_id; (void)required_scope;
    out_result->allowed = false;
    out_result->status = BHARAT_CAP_INVALID;

    if (handle == 99) { // 99 represents privileged administrator
        out_result->allowed = true;
        out_result->status = BHARAT_CAP_OK;
        return BHARAT_CAP_OK;
    }

    if (expected_object_type == BHARAT_CAP_OBJ_DEVICE) {
        if (required_rights == BH_CAP_RIGHT_READ) { // ENUMERATE / OPEN DEVICE
            if (handle == 100 || handle == 101) {
                out_result->allowed = true;
                out_result->status = BHARAT_CAP_OK;
                return BHARAT_CAP_OK;
            }
        }
        if (required_rights == BH_CAP_RIGHT_CREATE) { // CREATE QUEUE
            if (handle == 101) {
                out_result->allowed = true;
                out_result->status = BHARAT_CAP_OK;
                return BHARAT_CAP_OK;
            }
        }
        if (required_rights == BH_CAP_RIGHT_EXECUTE) { // SUBMIT
            if (handle == 101) {
                out_result->allowed = true;
                out_result->status = BHARAT_CAP_OK;
                return BHARAT_CAP_OK;
            }
        }
    }

    if (expected_object_type == BHARAT_CAP_OBJ_DMA_DOMAIN) {
        if (required_rights == BH_CAP_RIGHT_MAP) { // REGISTER BUFFER
            if (handle == 101) {
                out_result->allowed = true;
                out_result->status = BHARAT_CAP_OK;
                return BHARAT_CAP_OK;
            }
        }
    }

    return BHARAT_CAP_INVALID;
}

static void test_accelmgr_broker_lifecycle(void) {
    init_accelmgr();
    bharat_cap_set_validate_backend_for_tests(mock_cap_validate);

    // 1. Admission
    bharat_device_accelmgr_v2_RequestAdmissionReq_t adm_req = {0};
    bharat_device_accelmgr_v2_RequestAdmissionResp_t adm_resp = {0};

    // Unprivileged context (handle 50) - gets rejected
    int ret = handle_RequestAdmission(&adm_req, &adm_resp, 50, 101);
    assert(ret == 0);
    assert(adm_resp.status == -13); // Permission denied

    // Privileged context (handle 101) - accepted
    ret = handle_RequestAdmission(&adm_req, &adm_resp, 101, 101);
    assert(ret == 0);
    assert(adm_resp.status == 0);
    uint64_t ctx_handle = adm_resp.context_handle;
    assert(ctx_handle != 0);

    // 2. Register buffer
    bharat_device_accelmgr_v2_RegisterBufferReq_t buf_req = {0};
    bharat_device_accelmgr_v2_RegisterBufferResp_t buf_resp = {0};
    buf_req.size = 4096;
    buf_req.memory_cap_handle = 101;

    // Register with unprivileged client (handle 50) - rejected
    ret = handle_RegisterBuffer(&buf_req, &buf_resp, 50, 101);
    assert(buf_resp.status == -13);

    // Register with privileged client (handle 101) - accepted
    ret = handle_RegisterBuffer(&buf_req, &buf_resp, 101, 101);
    assert(buf_resp.status == 0);
    uint64_t in_buf_handle = buf_resp.buffer_handle;

    buf_req.size = 4096;
    ret = handle_RegisterBuffer(&buf_req, &buf_resp, 101, 101);
    assert(buf_resp.status == 0);
    uint64_t out_buf_handle = buf_resp.buffer_handle;

    // Create NPU queue
    bharat_device_accelmgr_v2_CreateQueueReq_t q_req = {
        .context_handle = ctx_handle,
        .device_handle = g_npu_handle,
        .depth = 16,
        .priority = 1
    };
    bharat_device_accelmgr_v2_CreateQueueResp_t q_resp = {0};
    ret = handle_CreateQueue(&q_req, &q_resp, 101, 101);
    assert(q_resp.status == 0);
    uint64_t q_handle = q_resp.queue_handle;

    // Buffer range checks & integer-overflow checking
    bharat_device_accelmgr_v2_SubmitJobReq_t sub_req = {0};
    bharat_device_accelmgr_v2_SubmitJobResp_t sub_resp = {0};

    sub_req.queue_handle = q_handle;
    sub_req.input_buffer_handle = in_buf_handle;
    sub_req.input_offset = 2000;
    sub_req.input_length = 3000; // 2000 + 3000 = 5000 > 4096 (Overflow/bounds check)
    sub_req.output_buffer_handle = out_buf_handle;
    sub_req.output_offset = 0;
    sub_req.output_length = 1000;

    ret = handle_SubmitJob(&sub_req, &sub_resp, 101, 101);
    assert(sub_resp.status == -17); // Range overflow rejected!

    // Integer overflow checks
    sub_req.input_offset = 0xFFFFFFFFFFFFFFF0ULL;
    sub_req.input_length = 0x20ULL; // overflows past 0
    ret = handle_SubmitJob(&sub_req, &sub_resp, 101, 101);
    assert(sub_resp.status == -17); // Integer overflow rejected!

    // Clean up
    bharat_device_accelmgr_v2_ReleaseContextReq_t rel_req = {.context_handle = ctx_handle};
    bharat_device_accelmgr_v2_ReleaseContextResp_t rel_resp = {0};
    ret = handle_ReleaseContext(&rel_req, &rel_resp, 101, 101);
    assert(rel_resp.status == 0);
}

int main(void) {
    printf("Running test_accelmgr_broker...\n");
    test_accelmgr_broker_lifecycle();
    printf("test_accelmgr_broker PASSED\n");
    return 0;
}
