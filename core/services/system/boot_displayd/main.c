#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "bharat/ui/tiny_ui.h"
#include "bharat/uapi/display/boot_display.h"
#include "bharat/uapi/display/lease.h"
#include "bharat/runtime/runtime.h"

#define DISPLAY_BROKER_ENDPOINT 15 // Simulated well-known endpoint

typedef struct {
    bharat_display_lease_id_t lease_id;
    bharat_tiny_fb_t fb;
} boot_display_ctx_t;

static int boot_display_acquire_lease(boot_display_ctx_t *ctx) {
    struct { uint32_t display_id; uint32_t requested_rights; } req;
    struct { uint32_t status; uint32_t lease_id; uint32_t granted_rights; uint64_t fb_ptr; } resp;

    req.display_id = 1;
    req.requested_rights = BHARAT_DISPLAY_RIGHT_LEASE | BHARAT_DISPLAY_RIGHT_WRITE | BHARAT_DISPLAY_RIGHT_PRESENT;

    bharat_ipc_msg_header_t req_hdr = {0};
    req_hdr.opcode = 1; // RequestLease
    req_hdr.payload_size = sizeof(req);

    bharat_ipc_msg_header_t resp_hdr;

    // In production, we'd lookup DISPLAY_BROKER_ENDPOINT via namesvc
    int32_t ret = bharat_ipc_call(DISPLAY_BROKER_ENDPOINT, &req_hdr, &req, &resp_hdr, &resp, sizeof(resp));

    if (ret == 0 && resp.status == 0) {
        ctx->lease_id = resp.lease_id;
        ctx->fb.width_px = 800;
        ctx->fb.height_px = 480;
        ctx->fb.stride_bytes = 800 * 4;
        ctx->fb.pixel_format = BHARAT_UI_PIXEL_FMT_XRGB8888;
        ctx->fb.pixels = (void*)resp.fb_ptr;
        return 0;
    }

    return -1;
}

int main(void) {
    boot_display_ctx_t ctx;
    bharat_runtime_log("boot_displayd starting");

    if (boot_display_acquire_lease(&ctx) != 0) {
        bharat_runtime_log("failed to acquire display lease");
        return 1;
    }

    bharat_tiny_ui_state_t ui_state;
    bharat_tiny_ui_init(&ui_state, false);

    for (unsigned tick = 0; tick < 10; ++tick) {
        ui_state.progress_percent = (uint8_t)(tick * 10);
        bharat_tiny_ui_render(&ctx.fb, &ui_state);

        // In real system, we'd send Present IPC to broker here
    }

    bharat_runtime_log("boot_displayd handoff");
    // ReleaseLease IPC...

    return 0;
}
