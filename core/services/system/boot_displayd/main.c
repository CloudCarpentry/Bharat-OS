#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include "bharat/ui/tiny_ui.h"
#include "bharat/uapi/display/boot_display.h"
#include "bharat/uapi/display/display_v2.h"
#include "bharat/uapi/display/lease.h"
#include "bharat/runtime/runtime.h"
#include <bharat/ipc/ipc.h>
#include <bharat/cap/cap.h>

#ifdef BHARAT_UI_LVGL
#include "bharat_lvgl.h"
#include "bharat_shell.h"
#include "lvgl.h"
extern lv_display_t *bharat_lvgl_display_create(bh_display_lease_handle_t lease, uint32_t width, uint32_t height);
extern lv_indev_t *bharat_lvgl_pointer_create(void);
extern lv_indev_t *bharat_lvgl_keyboard_create(void);

static void demo_snapshot(bh_shell_system_info_t *info, void *context) {
  (void)context;
  info->uptime_seconds = bharat_lvgl_now_ms() / 1000U;
}
#endif

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

#ifdef BHARAT_UI_LVGL
    bharat_runtime_log("boot_displayd: using LVGL rich animated GUI with kernel logs");
    lv_init();
    bharat_lvgl_tick_init();

    lv_display_t *disp = bharat_lvgl_display_create(ctx.lease_id, ctx.fb.width_px, ctx.fb.height_px);
    if (!disp) {
        bharat_runtime_log("failed to create LVGL display, falling back to tiny_ui");
        goto fallback_tiny_ui;
    }

    bh_shell_set_snapshot_provider(demo_snapshot, NULL);
    bh_shell_start();

    /* Splash screen shows progress */
    for (unsigned tick = 0; tick < 10; ++tick) {
        // Here we'd normally receive IPC messages about kernel boot progress
        // and log them to the UI. Since we are using the shell_ui splash,
        // we'll pump LVGL timers.
        uint32_t delay = lv_timer_handler();
        bharat_lvgl_wait_ms(delay > 0 ? delay : 100);
    }

    // Move to launcher or other screen after splash
    bh_shell_navigate(BH_SHELL_SCREEN_LAUNCHER);

    // Pump loop
    for (int i=0; i<10; i++) {
        uint32_t delay = lv_timer_handler();
        bharat_lvgl_wait_ms(delay > 0 ? delay : 100);
    }

    bharat_runtime_log("boot_displayd LVGL handoff");
    return 0;

fallback_tiny_ui:
#endif

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
