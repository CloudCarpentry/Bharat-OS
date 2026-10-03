#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "bharat/ui/tiny_ui.h"
#include "bharat/uapi/display/boot_display.h"
#include "bharat/uapi/display/display_v2.h"
#include "bharat/uapi/display/lease.h"
#include "bharat/runtime/runtime.h"
#include <bharat/ipc/ipc.h>
#include <bharat/cap/cap.h>

#include "display_client.h"

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

typedef enum {
    BOOT_STATE_UNAVAILABLE,
    BOOT_STATE_INITIALIZING,
    BOOT_STATE_SPLASH_ACTIVE,
    BOOT_STATE_RECOVERY,
    BOOT_STATE_HANDOFF_PENDING,
    BOOT_STATE_RELEASED,
    BOOT_STATE_FAILED
} boot_display_state_t;

typedef enum {
    BH_BOOT_EVENT_KERNEL_READY,
    BH_BOOT_EVENT_INIT_STARTED,
    BH_BOOT_EVENT_SERVICE_SPAWNED,
    BH_BOOT_EVENT_SERVICE_READY,
    BH_BOOT_EVENT_SERVICE_FAILED,
    BH_BOOT_EVENT_DISPLAY_READY,
    BH_BOOT_EVENT_HANDOFF_COMPLETE
} bh_boot_event_t;

typedef struct {
    boot_display_state_t state;
    bh_showcase_display_session_t session;
    bh_gui_surface_handle_t surface;
    bh_gui_buffer_handle_t buffer;
    void *mapped_pixels;
    bharat_tiny_fb_t fb;
    uint8_t progress_percent;
    bool is_lvgl;
} boot_display_ctx_t;

static boot_display_ctx_t g_ctx = { .state = BOOT_STATE_UNAVAILABLE };

// Exposed for testing
boot_display_state_t boot_displayd_get_state(void) {
    return g_ctx.state;
}

void boot_displayd_set_state_for_test(boot_display_state_t state) {
    g_ctx.state = state;
}

static void release_resources(boot_display_ctx_t *ctx) {
    // We would use the release/destroy RPCs from V2 client here in a real implementation
    // For now, we update our local state tracking
    if (ctx->mapped_pixels) {
        free(ctx->mapped_pixels);
        ctx->mapped_pixels = NULL;
    }

    }

int boot_display_init(boot_display_ctx_t *ctx) {
    ctx->state = BOOT_STATE_INITIALIZING;

    bh_display_result_t res = bh_showcase_display_open(&ctx->session);
    if (res != BH_DISPLAY_RESULT_OK) {
        bharat_runtime_log("boot_displayd: display unavailable or lease denied");
        ctx->state = BOOT_STATE_UNAVAILABLE;
        return -1;
    }

    if (ctx->session.pixel_format != BH_DISPLAY_FORMAT_XRGB8888) {
        bharat_runtime_log("boot_displayd: unsupported pixel format");
        ctx->state = BOOT_STATE_FAILED;
        return -1;
    }

    res = bh_client_create_surface(ctx->session.lease, ctx->session.width, ctx->session.height, 0, &ctx->surface);
    if (res != BH_DISPLAY_RESULT_OK) {
        bharat_runtime_log("boot_displayd: failed to create surface");
        ctx->state = BOOT_STATE_FAILED;
        return -1;
    }

    bh_display_buffer_desc_t desc = {0};
    desc.width = ctx->session.width;
    desc.height = ctx->session.height;
    desc.pixel_format = ctx->session.pixel_format;
    desc.usage_flags = BH_DISPLAY_BUFFER_USAGE_CPU_WRITE | BH_DISPLAY_BUFFER_USAGE_SCANOUT;
    desc.memory_domain = BH_DISPLAY_MEMORY_DOMAIN_SYSTEM;
    desc.plane_count = 1;
    desc.modifier = BH_DISPLAY_MODIFIER_LINEAR;
    desc.planes[0].stride_bytes = ctx->session.width * 4;
    desc.planes[0].size_bytes = desc.planes[0].stride_bytes * ctx->session.height;
    desc.planes[0].offset_bytes = 0;
    desc.total_size_bytes = desc.planes[0].size_bytes;

    res = bh_client_register_buffer(ctx->session.lease, &desc, &ctx->buffer, &ctx->mapped_pixels);
    if (res != BH_DISPLAY_RESULT_OK) {
        bharat_runtime_log("boot_displayd: failed to register buffer");
        ctx->state = BOOT_STATE_FAILED;
        return -1;
    }

    res = bh_client_attach_buffer(ctx->session.lease, ctx->surface, ctx->buffer);
    if (res != BH_DISPLAY_RESULT_OK) {
        bharat_runtime_log("boot_displayd: failed to attach buffer");
        ctx->state = BOOT_STATE_FAILED;
        return -1;
    }

    ctx->fb.width_px = ctx->session.width;
    ctx->fb.height_px = ctx->session.height;
    ctx->fb.stride_bytes = desc.planes[0].stride_bytes;
    ctx->fb.pixel_format = BHARAT_UI_PIXEL_FMT_XRGB8888;
    ctx->fb.pixels = ctx->mapped_pixels;

    ctx->state = BOOT_STATE_SPLASH_ACTIVE;
    return 0;
}

void boot_display_handle_event(boot_display_ctx_t *ctx, bh_boot_event_t event) {
    if (ctx->state != BOOT_STATE_SPLASH_ACTIVE && ctx->state != BOOT_STATE_RECOVERY && ctx->state != BOOT_STATE_HANDOFF_PENDING) {
        return;
    }

    switch (event) {
        case BH_BOOT_EVENT_KERNEL_READY:
            ctx->progress_percent = 10;
            break;
        case BH_BOOT_EVENT_INIT_STARTED:
            ctx->progress_percent = 20;
            break;
        case BH_BOOT_EVENT_SERVICE_SPAWNED:
            if (ctx->progress_percent < 80) ctx->progress_percent += 10;
            break;
        case BH_BOOT_EVENT_SERVICE_READY:
            if (ctx->progress_percent < 90) ctx->progress_percent += 10;
            break;
        case BH_BOOT_EVENT_DISPLAY_READY:
            ctx->progress_percent = 100;
            ctx->state = BOOT_STATE_HANDOFF_PENDING;
            break;
        case BH_BOOT_EVENT_HANDOFF_COMPLETE:
            ctx->state = BOOT_STATE_RELEASED;
            release_resources(ctx);
            break;
        case BH_BOOT_EVENT_SERVICE_FAILED:
            ctx->state = BOOT_STATE_RECOVERY;
            break;
        default:
            break;
    }

    if (ctx->state == BOOT_STATE_SPLASH_ACTIVE || ctx->state == BOOT_STATE_RECOVERY) {
        if (!ctx->is_lvgl) {
            bharat_tiny_ui_state_t ui_state;
            bharat_tiny_ui_init(&ui_state, ctx->state == BOOT_STATE_RECOVERY);
            ui_state.progress_percent = ctx->progress_percent;
            bharat_tiny_ui_render(&ctx->fb, &ui_state);

            bh_gui_fence_handle_t fence = BH_GUI_HANDLE_INVALID;
            bh_client_present_surface(ctx->session.lease, ctx->surface, ctx->buffer, &fence);
        } else {
#ifdef BHARAT_UI_LVGL
            uint32_t delay = lv_timer_handler();
            bharat_lvgl_wait_ms(delay > 0 ? delay : 10);

            bh_gui_fence_handle_t fence = BH_GUI_HANDLE_INVALID;
            bh_client_present_surface(ctx->session.lease, ctx->surface, ctx->buffer, &fence);
#endif
        }
    }
}

#ifndef BHARAT_TESTING
int main(void) {
    bharat_runtime_log("boot_displayd starting");

    if (boot_display_init(&g_ctx) != 0) {
        bharat_runtime_log("boot_displayd running in headless/failed mode");
        return g_ctx.state == BOOT_STATE_FAILED ? 1 : 0;
    }

#ifdef BHARAT_UI_LVGL
    bharat_runtime_log("boot_displayd: attempting LVGL rich animated GUI");
    lv_init();
    bharat_lvgl_tick_init();

    lv_display_t *disp = bharat_lvgl_display_create(g_ctx.session.lease, g_ctx.fb.width_px, g_ctx.fb.height_px);
    if (disp) {
        g_ctx.is_lvgl = true;
        bh_shell_set_snapshot_provider(demo_snapshot, NULL);
        bh_shell_start();
    } else {
        bharat_runtime_log("failed to create LVGL display, falling back to tiny_ui");
        g_ctx.is_lvgl = false;
    }
#else
    g_ctx.is_lvgl = false;
#endif

    // Mock progress for now to simulate the boot sequence until Agent 1 provides real IPC events
    bh_boot_event_t mock_events[] = {
        BH_BOOT_EVENT_KERNEL_READY,
        BH_BOOT_EVENT_INIT_STARTED,
        BH_BOOT_EVENT_SERVICE_SPAWNED,
        BH_BOOT_EVENT_SERVICE_READY,
        BH_BOOT_EVENT_DISPLAY_READY,
        BH_BOOT_EVENT_HANDOFF_COMPLETE
    };

    for (size_t i = 0; i < sizeof(mock_events) / sizeof(mock_events[0]); i++) {
        boot_display_handle_event(&g_ctx, mock_events[i]);
        if (g_ctx.state == BOOT_STATE_RELEASED) {
            break;
        }
    }

    bharat_runtime_log("boot_displayd handoff complete");
    return 0;
}
#endif
