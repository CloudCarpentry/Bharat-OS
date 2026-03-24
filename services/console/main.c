#include <stdint.h>
#include <stddef.h>
#include <bharat/ipc/ipc.h>
#include <bharat/cap/cap.h>

/*
 * Bharat-OS User-Space Console Daemon
 *
 * Provides multiplexed TTY streams and logging over URPC to various
 * personalities and applications. This daemon assumes ownership of the UART/Display capabilities.
 */

// Well-known endpoint for the manager service that grants hardware capabilities
#define CONSOLE_MANAGER_ENDPOINT ((bharat_cap_handle_t)10)

// Definitions mirror console_v1.bidl operations
typedef struct {
    uint32_t opcode;
    uint32_t payload_len;
    uint32_t stream_id;
    uint32_t flags;
} console_msg_hdr_t;

typedef enum {
    CONSOLE_BACKEND_NONE = 0,
    CONSOLE_BACKEND_UART = 1,
    CONSOLE_BACKEND_FRAMEBUFFER = 2
} console_backend_kind_t;

typedef struct {
    uint32_t request_type;
    uint32_t preferred_backend;
} console_cap_request_t;

typedef struct {
    uint32_t status;
    uint32_t granted_backend;
} console_cap_response_t;

static void console_service_write(console_msg_hdr_t *hdr, const uint8_t *payload) {
    (void)hdr;
    (void)payload;
    // Skeleton: Write payload to the granted hardware capability
}

static void console_service_flush(uint32_t stream_id) {
    (void)stream_id;
    // Skeleton: Flush the underlying hardware capability
}

static bharat_cap_handle_t console_acquire_output_capability(console_backend_kind_t* granted_backend) {
    console_cap_request_t req;
    req.request_type = 1; // Generic 'request hardware cap' opcode
    req.preferred_backend = CONSOLE_BACKEND_UART; // Prefer UART initially

    bharat_ipc_msg_header_t send_hdr;
    send_hdr.message_id = 1; // MSG_ACQUIRE_CAP
    send_hdr.payload_size = sizeof(req);
    send_hdr.capability_transfer = BHARAT_CAP_INVALID_HANDLE;

    console_cap_response_t res;
    bharat_ipc_msg_header_t recv_hdr;
    recv_hdr.message_id = 0;
    recv_hdr.payload_size = 0;
    recv_hdr.capability_transfer = BHARAT_CAP_INVALID_HANDLE;

    // Send the capability request
    // int32_t ret = bharat_ipc_call(CONSOLE_MANAGER_ENDPOINT, &send_hdr, &req, &recv_hdr, &res, sizeof(res));
    int32_t ret = -1; // Mock failure until fully implemented
    res.status = 1;

    if (ret == 0 && res.status == 0) {
        if (granted_backend) {
            *granted_backend = (console_backend_kind_t)res.granted_backend;
        }
        return recv_hdr.capability_transfer;
    }

    if (granted_backend) {
        *granted_backend = CONSOLE_BACKEND_NONE;
    }
    return BHARAT_CAP_INVALID_HANDLE;
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    console_backend_kind_t backend = CONSOLE_BACKEND_NONE;
    bharat_cap_handle_t output_cap = console_acquire_output_capability(&backend);

    if (!bharat_cap_is_valid(output_cap)) {
        /*
         * We failed to acquire a valid hardware backend capability.
         * The daemon remains alive to handle future URPC requests or
         * retry initialization, but it cannot perform hardware output yet.
         */
    }

    // TODO: Initialize a ring buffer (URPC) for standard input/output streams
    // TODO: Poll incoming messages from Subsystem personalities (e.g. Linux printf)
    // TODO: Output characters to the active hardware backend

    while(1) {
        // Wait for incoming URPC messages containing log data or input requests
        // Dispatch to TTY sessions
    }

    return 0;
}
