#include <bharat/runtime/runtime.h>
#include <bharat/ipc/ipc.h>

int main(int argc, char **argv) {
    bharat_runtime_init();

    bharat_runtime_log("services/namesvc: Starting endpoint registry.");

    // TODO: Create the primary naming endpoint
    // TODO: Fetch delegated bootstrap capability from init

    bharat_runtime_log("services/namesvc: Registry initialized. Awaiting connections.");

    // Simulate main IPC loop handling registration/lookup requests
    while(1) {
        // Pseudo-code loop:
        // bharat_ipc_msg_header_t hdr;
        // char payload_buf[128];
        // int res = bharat_ipc_recv(my_endpoint, &hdr, payload_buf, sizeof(payload_buf));
        // if (res == 0) {
        //     handle_request(&hdr, payload_buf);
        // }
    }

    // Unreachable
    bharat_runtime_shutdown();
    return 0;
}
