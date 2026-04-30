#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "bharat/uapi/display/lease.h"
#include "bharat/uapi/display/display.h"
#include "bharat/uapi/ipc/status.h"
#include "bharat/uapi/ipc/manifest.h"
#include "bharat/runtime/runtime.h"
#include <bharat/service/service_runtime.h>
#include <bharat/ipc/ipc.h>

/**
 * Bharat-OS Display Broker Service
 *
 * DESIGN NOTE: This service implements the "transitional simulated shared framebuffer"
 * model. It manages display leases and provides clients with a simulated shared pointer
 * to the framebuffer memory. This model is intended to be replaced by VM-backed
 * shared mapping capabilities in a future phase.
 */

#define MAX_LEASES 4
#define MAX_DISPLAYS 2
#define FB_WIDTH 800
#define FB_HEIGHT 480
#define FB_SIZE_PX (FB_WIDTH * FB_HEIGHT)

typedef struct {
    bharat_display_lease_id_t id;
    uint32_t display_id;
    uint32_t rights;
    bharat_display_lease_state_t state;
    bharat_cap_handle_t client_endpoint;
} display_lease_t;

typedef struct {
    uint32_t id;
    bharat_display_mode_t current_mode;
    bharat_display_lease_id_t active_lease;
    bool discovered;
    uint32_t *fb_mem;
} display_device_t;

static display_lease_t g_leases[MAX_LEASES];
static display_device_t g_displays[MAX_DISPLAYS];
static uint32_t g_next_lease_id = 1;

// Global simulated shared framebuffer memory
static uint32_t g_fb_memory[FB_SIZE_PX];

static void broker_init(void) {
    for (int i = 0; i < MAX_LEASES; i++) {
        g_leases[i].state = BHARAT_DISPLAY_LEASE_STATE_INACTIVE;
        g_leases[i].id = BHARAT_DISPLAY_LEASE_INVALID;
    }
    for (int i = 0; i < MAX_DISPLAYS; i++) {
        g_displays[i].discovered = false;
        g_displays[i].active_lease = BHARAT_DISPLAY_LEASE_INVALID;
        g_displays[i].fb_mem = g_fb_memory;
    }

    g_displays[0].id = 1;
    g_displays[0].discovered = true;
    g_displays[0].current_mode.width = FB_WIDTH;
    g_displays[0].current_mode.height = FB_HEIGHT;
    g_displays[0].current_mode.pixel_format = 1; // XRGB8888
}

static void send_reply(bharat_cap_handle_t target, const bharat_ipc_msg_header_t *orig_hdr, bharat_status_t status, void *payload, uint32_t payload_size) {
    bharat_ipc_msg_header_t reply_hdr = *orig_hdr;
    reply_hdr.flags |= BHARAT_IPC_FLAG_REPLY;
    reply_hdr.status = status;
    reply_hdr.payload_size = payload_size;
    bharat_ipc_send(target, &reply_hdr, payload);
}

static void handle_request_lease(const bharat_ipc_msg_header_t *hdr, const void *payload) {
    if (hdr->payload_size < 8) {
        send_reply(hdr->reply_endpoint, hdr, BHARAT_IPC_STATUS_ERR_LENGTH, NULL, 0);
        return;
    }

    const struct { uint32_t display_id; uint32_t requested_rights; } *req = payload;
    struct { uint32_t status; uint32_t lease_id; uint32_t granted_rights; uint64_t fb_ptr; } resp;

    uint32_t disp_id = req->display_id;
    if (disp_id == 0 || disp_id > MAX_DISPLAYS || !g_displays[disp_id-1].discovered) {
        resp.status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
        send_reply(hdr->reply_endpoint, hdr, BHARAT_IPC_STATUS_ERR_NOT_FOUND, &resp, sizeof(resp));
        return;
    }

    display_device_t *disp = &g_displays[disp_id-1];

    // Policy: Revoke existing lease on new request for now (handoff)
    if (disp->active_lease != BHARAT_DISPLAY_LEASE_INVALID) {
         for (int i=0; i<MAX_LEASES; i++) {
             if (g_leases[i].id == disp->active_lease) {
                 g_leases[i].state = BHARAT_DISPLAY_LEASE_STATE_REVOKED;
                 break;
             }
         }
    }

    int found = -1;
    for (int i=0; i<MAX_LEASES; i++) {
        if (g_leases[i].state == BHARAT_DISPLAY_LEASE_STATE_INACTIVE || g_leases[i].state == BHARAT_DISPLAY_LEASE_STATE_REVOKED) {
            found = i; break;
        }
    }

    if (found != -1) {
        g_leases[found].id = g_next_lease_id++;
        g_leases[found].display_id = disp_id;
        g_leases[found].rights = req->requested_rights;
        g_leases[found].state = BHARAT_DISPLAY_LEASE_STATE_ACTIVE;
        g_leases[found].client_endpoint = hdr->reply_endpoint;

        disp->active_lease = g_leases[found].id;
        resp.status = BHARAT_IPC_STATUS_OK;
        resp.lease_id = g_leases[found].id;
        resp.granted_rights = g_leases[found].rights;
        resp.fb_ptr = (uint64_t)disp->fb_mem;
        send_reply(hdr->reply_endpoint, hdr, BHARAT_STATUS_OK, &resp, sizeof(resp));
    } else {
        resp.status = BHARAT_IPC_STATUS_ERR_INTERNAL;
        send_reply(hdr->reply_endpoint, hdr, BHARAT_STATUS_ERR_INTERNAL, &resp, sizeof(resp));
    }
}

static void handle_present(const bharat_ipc_msg_header_t *hdr, const void *payload) {
    if (hdr->payload_size < 8) {
        send_reply(hdr->reply_endpoint, hdr, BHARAT_IPC_STATUS_ERR_LENGTH, NULL, 0);
        return;
    }

    const struct { uint32_t lease_id; uint32_t buffer_handle; } *req = payload;

    // Validate lease
    int lease_idx = -1;
    for (int i=0; i<MAX_LEASES; i++) {
        if (g_leases[i].id == req->lease_id) {
            lease_idx = i; break;
        }
    }

    if (lease_idx == -1 || g_leases[lease_idx].state != BHARAT_DISPLAY_LEASE_STATE_ACTIVE) {
        send_reply(hdr->reply_endpoint, hdr, BHARAT_IPC_STATUS_ERR_PERM, NULL, 0);
        return;
    }

    if (!(g_leases[lease_idx].rights & BHARAT_DISPLAY_RIGHT_PRESENT)) {
        send_reply(hdr->reply_endpoint, hdr, BHARAT_IPC_STATUS_ERR_PERM, NULL, 0);
        return;
    }

    send_reply(hdr->reply_endpoint, hdr, BHARAT_STATUS_OK, NULL, 0);
}

bharat_status_t bh_service_handle_msg(bh_service_ctx_t *ctx, const bh_msg_t *msg) {
    (void)ctx;
    switch (msg->header.opcode) {
        case 1: // RequestLease
            handle_request_lease(&msg->header, msg->payload);
            break;
        case 2: // ReleaseLease
            send_reply(msg->header.reply_endpoint, &msg->header, BHARAT_STATUS_OK, NULL, 0);
            break;
        case 3: // Present
            handle_present(&msg->header, msg->payload);
            break;
        default:
            send_reply(msg->header.reply_endpoint, &msg->header, BHARAT_IPC_STATUS_ERR_OPCODE, NULL, 0);
            break;
    }
    return BHARAT_STATUS_OK;
}

int main(void) {
    bharat_runtime_init();
    broker_init();
    bharat_runtime_log("Display Broker started (transitional shared-pointer model)");

    bh_service_start_info_t info = {
        .service_id = 0x00010007,
        .service_name = "display_broker",
        .endpoint = BHARAT_CAP_INVALID_HANDLE // Should be acquired from namesvc
    };

    return bh_service_main(&info);
}
