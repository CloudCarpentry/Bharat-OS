#include <lib/base/string.h>
#include "headless_backend.h"
#include "../../compositor_core.h"

static bh_compositor_core_t g_legacy_core;
static bh_display_handle_t g_legacy_display;
static bh_display_lease_handle_t g_legacy_lease;
static bool g_legacy_initialized = false;

static uint64_t dummy_clock_now(void *ctx) {
    (void)ctx;
    static uint64_t time_ns = 1000;
    time_ns += 16666666ULL; /* 16.6ms */
    return time_ns;
}

static void ensure_legacy_initialized(void) {
    if (g_legacy_initialized) return;
    bh_profile_caps_t caps = {
        .profile_class = BH_PROFILE_CLASS_DESKTOP,
        .ui_enabled = true,
        .compositor_enabled = true,
        .direct_scanout_allowed = true,
        .trusted_overlay_allowed = true
    };
    bh_gui_clock_t clock = {
        .now_ns = dummy_clock_now,
        .ctx = NULL
    };
    bh_compositor_core_init(&g_legacy_core, &caps, &clock);

    /* Setup 1 display and 1 lease */
    bh_compositor_add_display(&g_legacy_core, 800, 600, 60, BH_DISPLAY_FORMAT_XRGB8888, &g_legacy_display);
    bh_plane_desc_t plane = {
        .supported_formats_mask = 0xF,
        .min_width = 1,
        .min_height = 1,
        .max_width = 4096,
        .max_height = 4096,
        .direct_scanout_support = true,
        .z_order_min = 0,
        .z_order_max = 10
    };
    bh_compositor_add_display_plane(&g_legacy_core, g_legacy_display, &plane);
    bh_compositor_request_lease(&g_legacy_core, g_legacy_display, BHARAT_DISPLAY_RIGHT_PRESENT | BHARAT_DISPLAY_RIGHT_WRITE, 100, 0, &g_legacy_lease);

    g_legacy_initialized = true;
}

void bharat_headless_compositor_init(bharat_headless_compositor_t *ctx) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));
    ensure_legacy_initialized();
}

int32_t bharat_headless_compositor_create_surface(bharat_headless_compositor_t *ctx, uint32_t w, uint32_t h, uint64_t *id) {
    ensure_legacy_initialized();
    if (ctx->surface_count >= MAX_SURFACES) return -1;

    bh_gui_surface_handle_t surf_h;
    bh_display_result_t res = bh_compositor_create_surface(&g_legacy_core, g_legacy_lease, w, h, ctx->surface_count, &surf_h);
    if (res != BH_DISPLAY_RESULT_OK) return -1;

    uint32_t idx = ctx->surface_count++;
    uint16_t slot = bh_gui_handle_get_slot(surf_h);

    ctx->surfaces[idx].surface_id = slot; /* Return slot index (e.g. 1) for legacy test compatibility */
    ctx->surfaces[idx].width = w;
    ctx->surfaces[idx].height = h;
    ctx->surfaces[idx].state = BHARAT_SURFACE_STATE_CREATED;

    if (id) *id = slot;
    return 0;
}

int32_t bharat_headless_compositor_submit_buffer(bharat_headless_compositor_t *ctx, uint64_t surface_id, uint64_t buffer_id) {
    ensure_legacy_initialized();

    /* Find index for the surface_id (slot) */
    int surf_idx = -1;
    for (uint32_t i = 0; i < ctx->surface_count; i++) {
        if (ctx->surfaces[i].surface_id == surface_id) {
            surf_idx = (int)i;
            break;
        }
    }
    if (surf_idx == -1) return -1;

    uint32_t core_surf_idx = (uint32_t)surface_id - 1;
    if (core_surf_idx >= MAX_SURFACES || !g_legacy_core.surfaces[core_surf_idx].active) return -1;
    bh_gui_surface_handle_t surf_h = g_legacy_core.surfaces[core_surf_idx].handle;

    /* Register a dummy buffer and attach it using compositor core */
    bh_display_buffer_desc_t desc = {
        .width = ctx->surfaces[surf_idx].width,
        .height = ctx->surfaces[surf_idx].height,
        .pixel_format = BH_DISPLAY_FORMAT_XRGB8888,
        .usage_flags = BH_DISPLAY_BUFFER_USAGE_SCANOUT,
        .memory_domain = BH_DISPLAY_MEMORY_DOMAIN_SYSTEM,
        .plane_count = 1,
        .total_size_bytes = ctx->surfaces[surf_idx].width * ctx->surfaces[surf_idx].height * 4,
        .modifier = BH_DISPLAY_MODIFIER_LINEAR,
        .planes = {
            { .offset_bytes = 0, .size_bytes = ctx->surfaces[surf_idx].width * ctx->surfaces[surf_idx].height * 4, .stride_bytes = ctx->surfaces[surf_idx].width * 4 }
        }
    };

    bh_gui_buffer_handle_t buf_h;
    bh_display_result_t res = bh_compositor_register_buffer(&g_legacy_core, g_legacy_lease, &desc, &buf_h);
    if (res != BH_DISPLAY_RESULT_OK) return -1;

    res = bh_compositor_attach_buffer(&g_legacy_core, g_legacy_lease, surf_h, buf_h);
    if (res != BH_DISPLAY_RESULT_OK) return -1;

    ctx->active_buffers[surf_idx] = buffer_id;

    /* Perform present */
    bh_direct_scanout_reason_t reason;
    res = bh_compositor_present(&g_legacy_core, g_legacy_lease, surf_h, buf_h, BH_GUI_HANDLE_INVALID, 0, &reason, NULL);
    if (res != BH_DISPLAY_RESULT_OK) return -1;

    return 0;
}

void bharat_headless_compositor_present(bharat_headless_compositor_t *ctx) {
    ctx->frame_counter++;
}
