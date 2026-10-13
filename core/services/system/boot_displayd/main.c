/* SPDX-License-Identifier: MIT */
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
#include "bharat/uapi/boot/boot_events.h"
#include "bharat/runtime/runtime.h"
#include <bharat/ipc/ipc.h>
#include <bharat/cap/cap.h>
#include <bharat/syscalls.h>
#include <bharat/uapi/syscall/bh_syscall_numbers.h>
#include <bharat/uapi/syscall/bh_syscall.h>

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

/* Exposed for testing */
boot_display_state_t boot_displayd_get_state(void) {
    return g_ctx.state;
}

void boot_displayd_set_state_for_test(boot_display_state_t state) {
    g_ctx.state = state;
}

static void release_resources(boot_display_ctx_t *ctx) {
    if (!ctx) return;

    if (ctx->session.lease != BH_GUI_HANDLE_INVALID) {
        if (ctx->buffer != BH_GUI_HANDLE_INVALID) {
            bh_client_release_buffer(ctx->session.lease, ctx->buffer);
            ctx->buffer = BH_GUI_HANDLE_INVALID;
        }
        if (ctx->surface != BH_GUI_HANDLE_INVALID) {
            bh_client_destroy_surface(ctx->session.lease, ctx->surface);
            ctx->surface = BH_GUI_HANDLE_INVALID;
        }
        bh_client_release_lease(ctx->session.lease);
        ctx->session.lease = BH_GUI_HANDLE_INVALID;
    }

    if (ctx->mapped_pixels) {
        free(ctx->mapped_pixels);
        ctx->mapped_pixels = NULL;
    }
    ctx->fb.pixels = NULL;
}

int boot_display_init(boot_display_ctx_t *ctx) {
    if (!ctx) return -1;
    ctx->state = BOOT_STATE_INITIALIZING;

    bh_display_result_t res = bh_showcase_display_open(&ctx->session);
    if (res != BH_DISPLAY_RESULT_OK) {
        bharat_runtime_log("boot_displayd: display unavailable or lease denied\n");
        ctx->state = BOOT_STATE_UNAVAILABLE;
        return -1;
    }

    if (ctx->session.pixel_format != BH_DISPLAY_FORMAT_XRGB8888) {
        bharat_runtime_log("boot_displayd: unsupported pixel format\n");
        ctx->state = BOOT_STATE_FAILED;
        return -1;
    }

    res = bh_client_create_surface(ctx->session.lease, ctx->session.width, ctx->session.height, 0, &ctx->surface);
    if (res != BH_DISPLAY_RESULT_OK) {
        bharat_runtime_log("boot_displayd: failed to create surface\n");
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
        bharat_runtime_log("boot_displayd: failed to register buffer\n");
        ctx->state = BOOT_STATE_FAILED;
        return -1;
    }

    res = bh_client_attach_buffer(ctx->session.lease, ctx->surface, ctx->buffer);
    if (res != BH_DISPLAY_RESULT_OK) {
        bharat_runtime_log("boot_displayd: failed to attach buffer\n");
        ctx->state = BOOT_STATE_FAILED;
        return -1;
    }

    ctx->fb.width_px = ctx->session.width;
    ctx->fb.height_px = ctx->session.height;
    ctx->fb.stride_bytes = desc.planes[0].stride_bytes;
    ctx->fb.pixel_format = BHARAT_UI_PIXEL_FMT_XRGB8888;
    ctx->fb.pixels = ctx->mapped_pixels;
    ctx->progress_percent = 0;

    ctx->state = BOOT_STATE_SPLASH_ACTIVE;
    return 0;
}

uint8_t boot_display_stage_to_progress(bh_boot_stage_t stage) {
    switch (stage) {
        case BH_BOOT_STAGE_EARLY: return 10;
        case BH_BOOT_STAGE_HAL: return 20;
        case BH_BOOT_STAGE_SECURITY: return 30;
        case BH_BOOT_STAGE_MEMORY: return 45;
        case BH_BOOT_STAGE_SCHEDULER: return 60;
        case BH_BOOT_STAGE_DRIVERS: return 75;
        case BH_BOOT_STAGE_SERVICES: return 85;
        case BH_BOOT_STAGE_USERSPACE: return 95;
        case BH_BOOT_STAGE_READY: return 100;
        default: return 0;
    }
}

static void boot_display_render_frame(boot_display_ctx_t *ctx) {
    if (!ctx || !ctx->fb.pixels) return;

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
        (void)delay;
        bh_gui_fence_handle_t fence = BH_GUI_HANDLE_INVALID;
        bh_client_present_surface(ctx->session.lease, ctx->surface, ctx->buffer, &fence);
#endif
    }
}

void boot_display_handle_record(boot_display_ctx_t *ctx, const bh_boot_event_record_t *rec) {
    if (!ctx || !rec) return;
    if (ctx->state != BOOT_STATE_SPLASH_ACTIVE &&
        ctx->state != BOOT_STATE_RECOVERY &&
        ctx->state != BOOT_STATE_HANDOFF_PENDING) {
        return;
    }

    if (rec->status == BH_BOOT_STATUS_ERROR || rec->stage == BH_BOOT_STAGE_FAILURE) {
        ctx->state = BOOT_STATE_RECOVERY;
    } else if (rec->stage == BH_BOOT_STAGE_READY && rec->status == BH_BOOT_STATUS_OK) {
        ctx->progress_percent = 100;
        ctx->state = BOOT_STATE_HANDOFF_PENDING;
    } else {
        uint8_t stage_pct = boot_display_stage_to_progress(rec->stage);
        if (stage_pct > ctx->progress_percent) {
            ctx->progress_percent = stage_pct;
        }
    }

    if (ctx->state == BOOT_STATE_SPLASH_ACTIVE || ctx->state == BOOT_STATE_RECOVERY) {
        boot_display_render_frame(ctx);
    }
}

void boot_display_update_from_snapshot(boot_display_ctx_t *ctx, const bh_boot_event_snapshot_t *snapshot) {
    if (!ctx || !snapshot) return;
    if (ctx->state != BOOT_STATE_SPLASH_ACTIVE &&
        ctx->state != BOOT_STATE_RECOVERY &&
        ctx->state != BOOT_STATE_HANDOFF_PENDING) {
        return;
    }

    bh_boot_stage_t max_stage = BH_BOOT_STAGE_EARLY;
    bool has_events = (snapshot->count > 0);
    bool has_error = false;
    bool is_ready = false;

    for (uint32_t i = 0; i < snapshot->count; ++i) {
        const bh_boot_event_record_t *ev = &snapshot->events[i];
        if (ev->status == BH_BOOT_STATUS_ERROR || ev->stage == BH_BOOT_STAGE_FAILURE) {
            has_error = true;
        }
        if (ev->stage > max_stage && ev->stage < BH_BOOT_STAGE_COUNT) {
            max_stage = ev->stage;
        }
        if (ev->stage == BH_BOOT_STAGE_READY && ev->status == BH_BOOT_STATUS_OK) {
            is_ready = true;
        }
    }

    if (has_error) {
        ctx->state = BOOT_STATE_RECOVERY;
    } else if (is_ready) {
        ctx->progress_percent = 100;
        ctx->state = BOOT_STATE_HANDOFF_PENDING;
    } else if (has_events) {
        uint8_t stage_pct = boot_display_stage_to_progress(max_stage);
        if (stage_pct > ctx->progress_percent) {
            ctx->progress_percent = stage_pct;
        }
    }

    if (ctx->state == BOOT_STATE_SPLASH_ACTIVE || ctx->state == BOOT_STATE_RECOVERY) {
        boot_display_render_frame(ctx);
    }
}

void boot_display_handoff(boot_display_ctx_t *ctx) {
    if (!ctx) return;
    ctx->state = BOOT_STATE_RELEASED;
    release_resources(ctx);
}

#ifndef BHARAT_TESTING
int main(void) {
    bharat_runtime_log("boot_displayd: starting\n");

    if (boot_display_init(&g_ctx) != 0) {
        bharat_runtime_log("boot_displayd: display broker unavailable; exiting in headless mode.\n");
        return 0; /* Preserves headless boot success */
    }

#ifdef BHARAT_UI_LVGL
    bharat_runtime_log("boot_displayd: attempting LVGL rich GUI\n");
    lv_init();
    bharat_lvgl_tick_init();

    lv_display_t *disp = bharat_lvgl_display_create(g_ctx.session.lease, g_ctx.fb.width_px, g_ctx.fb.height_px);
    if (disp) {
        g_ctx.is_lvgl = true;
        bh_shell_set_snapshot_provider(demo_snapshot, NULL);
        bh_shell_start();
    } else {
        bharat_runtime_log("boot_displayd: failed to create LVGL display, falling back to tiny_ui\n");
        g_ctx.is_lvgl = false;
    }
#else
    g_ctx.is_lvgl = false;
#endif

    /* Render initial baseline frame */
    boot_display_render_frame(&g_ctx);

    /* Bounded event loop querying truthful boot event snapshots */
    const unsigned max_iterations = 100; /* up to 2 seconds of splash before handoff */
    for (unsigned iter = 0; iter < max_iterations; ++iter) {
        bh_boot_event_snapshot_t snapshot;
        memset(&snapshot, 0, sizeof(snapshot));
        bh_boot_events_get_snapshot(&snapshot);

        boot_display_update_from_snapshot(&g_ctx, &snapshot);

        if (g_ctx.state == BOOT_STATE_HANDOFF_PENDING || g_ctx.state == BOOT_STATE_RELEASED) {
            break;
        }

        /* Non-busy sleep: 20ms per frame */
        bharat_syscall(BH_SYS_SCHED_SLEEP, 20, 0, 0, 0, 0, 0);
    }

    boot_display_handoff(&g_ctx);
    bharat_runtime_log("boot_displayd: handoff complete\n");
    return 0;
}
#endif
