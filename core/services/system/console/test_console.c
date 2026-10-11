#include <stdio.h>
#include <assert.h>
#include <string.h>

// Mock dependencies
#include <stdint.h>
#include <stdbool.h>

typedef uint32_t bharat_cap_handle_t;
typedef uint32_t bharat_status_t;
typedef uint32_t bharat_display_lease_id_t;

#define BHARAT_DISPLAY_RIGHT_LEASE   0x00000001u
#define BHARAT_DISPLAY_RIGHT_PRESENT 0x00000002u
#define BHARAT_DISPLAY_RIGHT_MODESET 0x00000004u
#define BHARAT_DISPLAY_RIGHT_READ    0x00000008u
#define BHARAT_DISPLAY_RIGHT_WRITE   0x00000010u

#define BHARAT_STATUS_OK 0
#define BHARAT_IPC_STATUS_ERR_OPCODE 1

#define BHARAT_CAP_INVALID_HANDLE 0xFFFFFFFF
#define BHARAT_IPC_FLAG_REPLY 0x01

typedef struct {
    uint32_t opcode;
    uint32_t payload_size;
    uint32_t flags;
    bharat_status_t status;
    bharat_cap_handle_t reply_endpoint;
    bharat_cap_handle_t capability_transfer;
} bharat_ipc_msg_header_t;

int bharat_ipc_call(bharat_cap_handle_t endpoint, bharat_ipc_msg_header_t *req_hdr, void *req, bharat_ipc_msg_header_t *resp_hdr, void *resp, uint32_t resp_size) {
    // Mock failure to test failing closed
    return -1;
}

void bharat_runtime_log(const char *msg) {
    printf("LOG: %s\n", msg);
}

int bharat_ipc_recv(bharat_cap_handle_t endpoint, bharat_ipc_msg_header_t *hdr, void *payload, uint32_t payload_size) {
    return -1;
}

void bharat_ipc_send(bharat_cap_handle_t target, bharat_ipc_msg_header_t *hdr, void *payload) {
}

// include main.c to test static logic
#define main console_main

#define _BHARAT_IPC_H_
#define _BHARAT_CAP_H_
#define BHARAT_UAPI_DISPLAY_LEASE_H
#define BHARAT_RUNTIME_H
#define BHARAT_IPC_STATUS_H
#include "main.c"

#undef main

int main() {
    console_init_fb();
    assert(g_fb_backend.active == false); // Should fail closed

    // Fake a valid init to test writes
    uint32_t pixels[800 * 480] = {0};
    g_fb_backend.width = 800;
    g_fb_backend.height = 480;
    g_fb_backend.fb_pixels = pixels;
    g_fb_backend.active = true;

    console_write_fb("Hello", 5);
    assert(g_fb_backend.cursor_x == 5);

    // Exceed bounds safety
    console_write_fb("\n", 1);
    assert(g_fb_backend.cursor_x == 0);
    assert(g_fb_backend.cursor_y == 1);

    printf("Console tests passed.\n");
    return 0;
}
