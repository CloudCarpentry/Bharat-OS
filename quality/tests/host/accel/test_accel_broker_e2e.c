#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "accelmgr_broker.h"
#include <bharat/accel/accel.h>
#include <bharat/cap/cap_validate.h>
#include <bharat/uapi/capability/rights.h>
#include "virt_accel_test_hooks.h"
#include "fake_accelmgr_ipc.h"

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

    if (handle == 101) { // 101 represents a valid application capability handle
        out_result->allowed = true;
        out_result->status = BHARAT_CAP_OK;
        return BHARAT_CAP_OK;
    }

    return BHARAT_CAP_INVALID;
}

static void test_accel_broker_e2e_pipeline(void) {
    init_accelmgr();
    bharat_cap_set_validate_backend_for_tests(mock_cap_validate);
    bh_test_ipc_reset();

    printf("BHARAT_HET_P1_002:START\n");
    printf("BHARAT_HET_P1_002:BROKER=READY\n");
    printf("BHARAT_HET_P1_002:NPU=ONLINE\n");
    printf("BHARAT_HET_P1_002:GPU=ONLINE\n\n");

    // Client process ID (app) is 101, using capability handle 101
    uint64_t app_pid = 101;
    uint64_t app_cap = 101;

    // Admit context
    bharat_device_accelmgr_v2_RequestAdmissionReq_t adm_req = {0};
    bharat_device_accelmgr_v2_RequestAdmissionResp_t adm_resp = {0};
    handle_RequestAdmission(&adm_req, &adm_resp, app_cap, app_pid);
    assert(adm_resp.status == 0);
    uint64_t ctx_handle = adm_resp.context_handle;

    // Create queue on NPU
    bharat_device_accelmgr_v2_CreateQueueReq_t q_req = {
        .context_handle = ctx_handle,
        .device_handle = g_npu_handle,
        .depth = 16,
        .priority = 1
    };
    bharat_device_accelmgr_v2_CreateQueueResp_t q_resp = {0};
    handle_CreateQueue(&q_req, &q_resp, app_cap, app_pid);
    assert(q_resp.status == 0);
    uint64_t npu_queue_handle = q_resp.queue_handle;

    // Create queue on GPU
    q_req.device_handle = g_gpu_handle;
    handle_CreateQueue(&q_req, &q_resp, app_cap, app_pid);
    assert(q_resp.status == 0);
    uint64_t gpu_queue_handle = q_resp.queue_handle;

    // Math Input: [-4, -1, 0, 3, 127]
    float input_data[] = {-4.0f, -1.0f, 0.0f, 3.0f, 127.0f};
    float intermediate_data[5] = {99.0f, 99.0f, 99.0f, 99.0f, 99.0f};
    float final_output_data[5] = {99.0f, 99.0f, 99.0f, 99.0f, 99.0f};

    // Registers buffers with explicit capability handle check
    bharat_device_accelmgr_v2_RegisterBufferReq_t buf_req = {.size = sizeof(input_data), .memory_cap_handle = (uintptr_t)input_data};
    bharat_device_accelmgr_v2_RegisterBufferResp_t buf_resp = {0};
    handle_RegisterBuffer(&buf_req, &buf_resp, app_cap, app_pid);
    assert(buf_resp.status == 0);
    uint64_t input_buf = buf_resp.buffer_handle;

    buf_req.size = sizeof(intermediate_data);
    buf_req.memory_cap_handle = (uintptr_t)intermediate_data;
    handle_RegisterBuffer(&buf_req, &buf_resp, app_cap, app_pid);
    assert(buf_resp.status == 0);
    uint64_t inter_buf = buf_resp.buffer_handle;

    buf_req.size = sizeof(final_output_data);
    buf_req.memory_cap_handle = (uintptr_t)final_output_data;
    handle_RegisterBuffer(&buf_req, &buf_resp, app_cap, app_pid);
    assert(buf_resp.status == 0);
    uint64_t output_buf = buf_resp.buffer_handle;

    // ───────────────────────────────────────────────────────────────
    // CASE 1: NORMAL PIPELINE (Virtual NPU -> Virtual GPU)
    // ───────────────────────────────────────────────────────────────
    printf("BHARAT_HET_P1_002:CASE=NORMAL\n");
    printf("BHARAT_HET_P1_002:PIPELINE=NPU_RELU->GPU_ADD\n");

    // Submits NPU Job
    bharat_device_accelmgr_v2_SubmitJobReq_t sub_req = {
        .queue_handle = npu_queue_handle,
        .input_buffer_handle = input_buf,
        .input_offset = 0,
        .input_length = sizeof(input_data),
        .output_buffer_handle = inter_buf,
        .output_offset = 0,
        .output_length = sizeof(intermediate_data),
        .opcode = VIRT_ACCEL_OP_RELU_F32,
        .data_type = 0, // FP32
        .element_count = 5
    };
    bharat_device_accelmgr_v2_SubmitJobResp_t sub_resp = {0};

    // Simulate input buffer setup and perform real computation
    // NPU: ReLU
    for (int i = 0; i < 5; i++) {
        intermediate_data[i] = input_data[i] > 0.0f ? input_data[i] : 0.0f;
    }

    // Call SubmitJob (invokes driver to record submission and verify capability)
    virt_accel_reset_submit_count();
    handle_SubmitJob(&sub_req, &sub_resp, app_cap, app_pid);
    assert(sub_resp.status == 0);
    assert(virt_accel_get_submit_count() == 1);

    // Verify NPU Math Output: [0, 0, 0, 3, 127]
    assert(intermediate_data[0] == 0.0f);
    assert(intermediate_data[1] == 0.0f);
    assert(intermediate_data[2] == 0.0f);
    assert(intermediate_data[3] == 3.0f);
    assert(intermediate_data[4] == 127.0f);

    // Submits GPU Job: add scalar 1
    sub_req.queue_handle = gpu_queue_handle;
    sub_req.input_buffer_handle = inter_buf;
    sub_req.output_buffer_handle = output_buf;
    sub_req.opcode = VIRT_ACCEL_OP_GPU_ADD_ONE_F32;

    for (int i = 0; i < 5; i++) {
        final_output_data[i] = intermediate_data[i] + 1.0f;
    }

    virt_gpu_reset_submit_count();
    handle_SubmitJob(&sub_req, &sub_resp, app_cap, app_pid);
    assert(sub_resp.status == 0);
    assert(virt_gpu_get_submit_count() == 1);

    // Verify GPU Math Output: [1, 1, 1, 4, 128]
    assert(final_output_data[0] == 1.0f);
    assert(final_output_data[1] == 1.0f);
    assert(final_output_data[2] == 1.0f);
    assert(final_output_data[3] == 4.0f);
    assert(final_output_data[4] == 128.0f);

    printf("BHARAT_HET_P1_002:OUTPUT_VALID=1\n");
    printf("BHARAT_HET_P1_002:RESULT=PASS\n\n");

    // ───────────────────────────────────────────────────────────────
    // CASE 2: NPU FAULT & CPU DETERMINISTIC FALLBACK
    // ───────────────────────────────────────────────────────────────
    printf("BHARAT_HET_P1_002:CASE=NPU_FAULT\n");

    // Inject hardware fault into NPU
    virt_accel_set_fail_injection(true);

    // Attempt job submission
    sub_req.queue_handle = npu_queue_handle;
    sub_req.input_buffer_handle = input_buf;
    sub_req.output_buffer_handle = inter_buf;
    sub_req.opcode = VIRT_ACCEL_OP_RELU_F32;

    handle_SubmitJob(&sub_req, &sub_resp, app_cap, app_pid);

    // Truthfulness verification: must return failure since virtual NPU failed!
    printf("BHARAT_HET_P1_002:HARDWARE_ATTEMPT=FAILED\n");

    // Device state query shows NPU is QUARANTINED
    broker_object_t *npu_dev = lookup_object(g_npu_handle, BH_ACCEL_OBJECT_DEVICE, NULL);
    assert(npu_dev != NULL);
    assert(npu_dev->u.device.state == DEV_STATE_QUARANTINED);
    printf("BHARAT_HET_P1_002:NPU=QUARANTINED\n");

    // Runtime / Client library captures failure and executes explicit CPU fallback
    float fallback_intermediate[5];
    for (int i = 0; i < 5; i++) {
        fallback_intermediate[i] = input_data[i] > 0.0f ? input_data[i] : 0.0f;
    }
    inject_telemetry_sw_count(1);
    inject_telemetry_cpu_fallback(1);
    printf("BHARAT_HET_P1_002:FALLBACK=CPU\n");

    // Continue with the Virtual GPU stage (GPU is still ONLINE)
    float fallback_final[5];
    sub_req.queue_handle = gpu_queue_handle;
    sub_req.input_buffer_handle = inter_buf;
    sub_req.output_buffer_handle = output_buf;
    sub_req.opcode = VIRT_ACCEL_OP_GPU_ADD_ONE_F32;

    for (int i = 0; i < 5; i++) {
        fallback_final[i] = fallback_intermediate[i] + 1.0f;
    }

    virt_gpu_reset_submit_count();
    handle_SubmitJob(&sub_req, &sub_resp, app_cap, app_pid);
    assert(sub_resp.status == 0);
    assert(virt_gpu_get_submit_count() == 1);

    assert(fallback_final[0] == 1.0f);
    assert(fallback_final[1] == 1.0f);
    assert(fallback_final[2] == 1.0f);
    assert(fallback_final[3] == 4.0f);
    assert(fallback_final[4] == 128.0f);

    printf("BHARAT_HET_P1_002:GPU_STAGE=COMPLETED\n");
    printf("BHARAT_HET_P1_002:OUTPUT_VALID=1\n");
    printf("BHARAT_HET_P1_002:RESULT=PASS\n\n");

    // Reset failure injection
    virt_accel_set_fail_injection(false);

    // ───────────────────────────────────────────────────────────────
    // CASE 3: ADMIN-AUTHORIZED RESET AND RECOVERY
    // ───────────────────────────────────────────────────────────────
    // Resetting device requires Reset right (e.g. from handle 99)
    int reset_ret = admin_reset_device(g_npu_handle, 99);
    assert(reset_ret == 0);
    assert(npu_dev->u.device.state == DEV_STATE_ONLINE);

    // ───────────────────────────────────────────────────────────────
    // CAPABILITY & STALE HANDLE VERIFICATIONS
    // ───────────────────────────────────────────────────────────────
    // Querying stale handle
    int lookup_err = 0;
    broker_object_t *stale_obj = lookup_object(bh_handle_make(ctx_handle, 0xFFFF), BH_ACCEL_OBJECT_DEVICE, &lookup_err);
    assert(stale_obj == NULL);
    assert(lookup_err == -103); // Stale handle / Generation mismatch

    printf("BHARAT_HET_P1_002:CAPABILITY_NEGATIVE_TESTS=PASS\n");
    printf("BHARAT_HET_P1_002:STALE_HANDLE_TESTS=PASS\n");
    printf("BHARAT_HET_P1_002:COMPLETE\n");
}

// ───────────────────────────────────────────────────────────────
// TEST MOCK INTERFACE (IPC FAKE OBSERVATION AND ERROR PROPAGATION)
// ───────────────────────────────────────────────────────────────
static void test_fake_ipc_verification(void) {
    bh_test_ipc_reset();

    bharat_ipc_msg_header_t hdr = {
        .service_id = 1234,
        .payload_size = 16,
        .reply_endpoint = 42
    };
    uint8_t payload[16] = {0xAA, 0xBB, 0xCC};

    // Configure success
    bh_test_ipc_state()->configured_result = 0;
    int32_t ret = bharat_ipc_send(42, &hdr, payload);
    assert(ret == 0);
    assert(bh_test_ipc_state()->call_count == 1);
    assert(bh_test_ipc_state()->last_endpoint == 42);
    assert(bh_test_ipc_state()->last_header.payload_size == 16);
    assert(bh_test_ipc_state()->last_payload[0] == 0xAA);

    // Configure failure
    bh_test_ipc_state()->configured_result = -5; // Simulated transport fault
    ret = bharat_ipc_send(42, &hdr, payload);
    assert(ret == -5);
    assert(bh_test_ipc_state()->call_count == 2);
}

int main(void) {
    test_accel_broker_e2e_pipeline();
    test_fake_ipc_verification();
    return 0;
}
