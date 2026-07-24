#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "ipc/contract_validate.h"
#include "ipc/ipc_status.h"
#include "ipc/ipc_traffic.h"
#include "bharat/urpc.h"
#include "hal/hal.h"

// Mock hal_cpu_get_id
uint32_t hal_cpu_get_id(void) {
    return 0;
}

// Mock urpc_bootstrap_send
int urpc_bootstrap_send(uint32_t target_core, uint64_t msg) {
    return 0;
}

// Mock urpc_mark_ready
void urpc_mark_ready(uint32_t source_core) {
}

static void test_ipc_contract_validation(void) {
    bharat_ipc_contract_header_t hdr = {
        .header_version = BHARAT_IPC_HEADER_VERSION_V1,
        .interface_version = 1,
        .opcode = 10,
        .payload_size = 64,
        .flags = 0
    };

    ipc_contract_rules_t rules = {
        .expected_version = 1,
        .min_opcode = 1,
        .max_opcode = 100,
        .max_payload_size = 128,
        .required_flags = 0,
        .allowed_flags = BHARAT_IPC_FLAG_REPLY | BHARAT_IPC_FLAG_NONBLOCK,
        .allow_variable_payload = true,
        .payload_alignment = 8
    };

    // Valid header
    assert(bharat_ipc_contract_validate_ex(&hdr, &rules) == BHARAT_IPC_STATUS_OK);

    // Invalid header version
    hdr.header_version = 0;
    assert(bharat_ipc_contract_validate_ex(&hdr, &rules) == BHARAT_IPC_STATUS_ERR_VERSION);
    hdr.header_version = BHARAT_IPC_HEADER_VERSION_V1;

    // Invalid interface version
    hdr.interface_version = 2;
    assert(bharat_ipc_contract_validate_ex(&hdr, &rules) == BHARAT_IPC_STATUS_ERR_VERSION);
    hdr.interface_version = 1;

    // Invalid opcode (too low)
    hdr.opcode = 0;
    assert(bharat_ipc_contract_validate_ex(&hdr, &rules) == BHARAT_IPC_STATUS_ERR_OPCODE);
    hdr.opcode = 10;

    // Invalid opcode (too high)
    hdr.opcode = 101;
    assert(bharat_ipc_contract_validate_ex(&hdr, &rules) == BHARAT_IPC_STATUS_ERR_OPCODE);
    hdr.opcode = 10;

    // Payload too large
    hdr.payload_size = 129;
    assert(bharat_ipc_contract_validate_ex(&hdr, &rules) == BHARAT_IPC_STATUS_ERR_LENGTH);
    hdr.payload_size = 64;

    // Payload alignment mismatch
    hdr.payload_size = 60;
    assert(bharat_ipc_contract_validate_ex(&hdr, &rules) == BHARAT_IPC_STATUS_ERR_DECODE);
    hdr.payload_size = 64;

    // Invalid flags (not allowed)
    hdr.flags = 0x10; // Reserved/unsupported
    assert(bharat_ipc_contract_validate_ex(&hdr, &rules) == BHARAT_IPC_STATUS_ERR_FLAGS);
    hdr.flags = 0;

    // Required flags missing
    rules.required_flags = BHARAT_IPC_FLAG_REPLY;
    assert(bharat_ipc_contract_validate_ex(&hdr, &rules) == BHARAT_IPC_STATUS_ERR_FLAGS);
    hdr.flags = BHARAT_IPC_FLAG_REPLY;
    assert(bharat_ipc_contract_validate_ex(&hdr, &rules) == BHARAT_IPC_STATUS_OK);
    rules.required_flags = 0;
    hdr.flags = 0;

    // Fixed payload size
    rules.allow_variable_payload = false;
    rules.max_payload_size = 64;
    assert(bharat_ipc_contract_validate_ex(&hdr, &rules) == BHARAT_IPC_STATUS_OK);
    hdr.payload_size = 32;
    assert(bharat_ipc_contract_validate_ex(&hdr, &rules) == BHARAT_IPC_STATUS_ERR_LENGTH);

    printf("test_ipc_contract_validation passed\n");
}

static void test_ipc_status_strings(void) {
    assert(strcmp(bharat_ipc_status_to_string(BHARAT_IPC_STATUS_OK), "IPC_OK") == 0);
    assert(strcmp(bharat_ipc_status_to_string(BHARAT_IPC_STATUS_ERR_VERSION), "IPC_ERR_VERSION") == 0);
    assert(strcmp(bharat_ipc_status_to_string(BHARAT_IPC_STATUS_ERR_FLAGS), "IPC_ERR_FLAGS") == 0);
    assert(strcmp(bharat_ipc_status_to_string((bharat_status_t)999), "IPC_ERR_UNKNOWN") == 0);
    printf("test_ipc_status_strings passed\n");
}

static void test_urpc_channel_validation(void) {
    // Test state transitions
    assert(urpc_channel_transition_allowed(URPC_CHANNEL_CLOSED, URPC_CHANNEL_BINDING) == true);
    assert(urpc_channel_transition_allowed(URPC_CHANNEL_CLOSED, URPC_CHANNEL_BOUND) == true);
    assert(urpc_channel_transition_allowed(URPC_CHANNEL_CLOSED, URPC_CHANNEL_ERROR) == false);

    assert(urpc_channel_transition_allowed(URPC_CHANNEL_BINDING, URPC_CHANNEL_BOUND) == true);
    assert(urpc_channel_transition_allowed(URPC_CHANNEL_BINDING, URPC_CHANNEL_ERROR) == true);
    assert(urpc_channel_transition_allowed(URPC_CHANNEL_BINDING, URPC_CHANNEL_CLOSED) == true);

    assert(urpc_channel_transition_allowed(URPC_CHANNEL_ERROR, URPC_CHANNEL_CLOSED) == true);
    assert(urpc_channel_transition_allowed(URPC_CHANNEL_ERROR, URPC_CHANNEL_BINDING) == false);

    // Test bind rejections
    assert(urpc_channel_bind(0) == BHARAT_IPC_STATUS_ERR_UNSUPPORTED); // Self-bind (mocked CPU ID is 0)
    assert(urpc_channel_bind(999) == BHARAT_IPC_STATUS_ERR_NOT_FOUND); // Invalid core

    assert(urpc_channel_bind(1) == BHARAT_IPC_STATUS_OK); // OK
    assert(urpc_channel_bind(1) == BHARAT_IPC_STATUS_ERR_INTERNAL); // Duplicate bind (already binding)

    // Test close
    assert(urpc_channel_close(1) == BHARAT_IPC_STATUS_OK);
    assert(urpc_channel_get_state(1) == URPC_CHANNEL_CLOSED);

    printf("test_urpc_channel_validation passed\n");
}

static void test_ipc_route_admission(void) {
    ipc_route_admission_t out;

    // Normal case (GP profile defaults)
    assert(ipc_route_admit(IPC_TRAFFIC_CONTROL, 16, false, &out) == BHARAT_IPC_STATUS_OK);
    assert(out.accepted == true);
    assert(out.reason == BHARAT_IPC_STATUS_OK);

    // Payload too large for endpoint
    assert(ipc_route_admit(IPC_TRAFFIC_BULK, 1024, false, &out) == BHARAT_IPC_STATUS_ERR_LENGTH);
    assert(out.accepted == false);
    assert(out.reason == BHARAT_IPC_STATUS_ERR_LENGTH);

    printf("test_ipc_route_admission passed\n");
}

int main(void) {
    test_ipc_contract_validation();
    test_ipc_status_strings();
    test_urpc_channel_validation();
    test_ipc_route_admission();
    printf("All IPC/URPC reliability tests passed.\n");
    return 0;
}
