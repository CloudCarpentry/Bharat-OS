#include <ipc/contract_validate.h>
#include <uapi/bharat/ipc/contract.h>
#include <uapi/bharat/ipc/status.h>
#include <assert.h>
#include <stdio.h>

void test_valid_header() {
    bharat_ipc_contract_header_t hdr = {
        .interface_version = 1,
        .opcode = 5,
        .payload_size = 32
    };

    int result = bharat_ipc_contract_validate(&hdr, 1, 0, 10, 32);
    assert(result == BHARAT_IPC_STATUS_OK);
    printf("test_valid_header passed\n");
}

void test_invalid_version() {
    bharat_ipc_contract_header_t hdr = {
        .interface_version = 2, // Expected 1
        .opcode = 5,
        .payload_size = 32
    };

    int result = bharat_ipc_contract_validate(&hdr, 1, 0, 10, 32);
    assert(result == BHARAT_IPC_STATUS_ERR_VERSION);
    printf("test_invalid_version passed\n");
}

void test_invalid_opcode() {
    bharat_ipc_contract_header_t hdr = {
        .interface_version = 1,
        .opcode = 15, // Max 10
        .payload_size = 32
    };

    int result = bharat_ipc_contract_validate(&hdr, 1, 0, 10, 32);
    assert(result == BHARAT_IPC_STATUS_ERR_OPCODE);
    printf("test_invalid_opcode passed\n");
}

void test_invalid_payload_size() {
    bharat_ipc_contract_header_t hdr = {
        .interface_version = 1,
        .opcode = 5,
        .payload_size = 16 // Expected 32
    };

    int result = bharat_ipc_contract_validate(&hdr, 1, 0, 10, 32);
    assert(result == BHARAT_IPC_STATUS_ERR_DECODE);
    printf("test_invalid_payload_size passed\n");
}

void test_variable_payload_size() {
    bharat_ipc_contract_header_t hdr = {
        .interface_version = 1,
        .opcode = 5,
        .payload_size = 128
    };

    // Expected size 0 means don't check exact size.
    int result = bharat_ipc_contract_validate(&hdr, 1, 0, 10, 0);
    assert(result == BHARAT_IPC_STATUS_OK);
    printf("test_variable_payload_size passed\n");
}

int main() {
    test_valid_header();
    test_invalid_version();
    test_invalid_opcode();
    test_invalid_payload_size();
    test_variable_payload_size();
    return 0;
}