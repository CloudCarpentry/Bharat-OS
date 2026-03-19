#include <bharat/runtime/runtime.h>
#include <stdio.h>
#include <stdlib.h>

void bharat_runtime_init(void) {
    // Stub implementation: initialize memory, TLS, thread structs
}

void bharat_runtime_shutdown(void) {
    // Stub implementation: clean up resources, close handles
}

bharat_cap_handle_t bharat_runtime_get_bootstrap_cap(void) {
    // Stub implementation
    return BHARAT_CAP_INVALID_HANDLE;
}

void bharat_runtime_log(const char *msg) {
    // Stub implementation: write to fd 1 or raw console for now
    if (msg) {
        // printf("%s\n", msg);
    }
}

void bharat_runtime_panic(const char *reason) {
    // Stub implementation: format breadcrumb and trap
    if (reason) {
        bharat_runtime_log("PANIC: ");
        bharat_runtime_log(reason);
    }
    while(1) {} // Unreachable
}

int bharat_runtime_main_wrapper(int argc, char **argv, int (*main_fn)(int, char**)) {
    // Boilerplate for native service loop
    bharat_runtime_init();

    int result = -1;
    if (main_fn) {
        result = main_fn(argc, argv);
    }

    bharat_runtime_shutdown();
    return result;
}
