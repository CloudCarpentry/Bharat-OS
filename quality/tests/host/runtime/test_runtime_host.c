#include "runtime_host.h"
#include <stdio.h>
#include <assert.h>

void test_runtime_host_invalid_handles(void) {
    bharat_runtime_handle_t out_handle = 123;
    int err;
    char dummy_buf[10];

    // File open with null paths
    err = bharat_runtime_file_open(NULL, 0, &out_handle);
    assert(err == -1);

    err = bharat_runtime_file_open("somepath", 0, NULL);
    assert(err == -1);

    // Test open assigns INVALID handle and returns -1
    err = bharat_runtime_file_open("somepath", 0, &out_handle);
    assert(out_handle == BHARAT_RUNTIME_INVALID_HANDLE);
    assert(err == -1);

    // Read invalid combinations
    assert(bharat_runtime_read(BHARAT_RUNTIME_INVALID_HANDLE, dummy_buf, 10) == -1);
    assert(bharat_runtime_read(1234, NULL, 10) == -1);

    // Read valid combination should return -1 unimplemented for now
    assert(bharat_runtime_read(1234, dummy_buf, 10) == -1);

    // Write invalid combinations
    assert(bharat_runtime_write(BHARAT_RUNTIME_INVALID_HANDLE, dummy_buf, 10) == -1);
    assert(bharat_runtime_write(1234, NULL, 10) == -1);

    // Write valid combination should return -1 unimplemented for now
    assert(bharat_runtime_write(1234, dummy_buf, 10) == -1);

    // Close invalid handles
    assert(bharat_runtime_close(BHARAT_RUNTIME_INVALID_HANDLE) == -1);

    // Close valid handle should return -1 unimplemented for now
    assert(bharat_runtime_close(1234) == -1);

    printf("All runtime host validation tests passed!\n");
}

int main(void) {
    test_runtime_host_invalid_handles();
    return 0;
}
