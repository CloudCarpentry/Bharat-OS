/* SPDX-License-Identifier: MIT */
#include "lvgl.h"
#include "bharat/uapi/display/display_v2.h"
#include "bharat/uapi/gui/surface_v2.h"
#include "bharat/uapi/display/bharat_display_broker_v2_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Mocked Display Broker Client IPC calls for this showcase/adapter.
   In a real implementation, these would route through BIDL IPC stubs. */

// We assume direct render model: 2 buffers
#define BUFFER_COUNT 2

typedef struct {
    bh_display_lease_handle_t lease;
    bh_gui_surface_handle_t surface;
    bh_gui_buffer_handle_t buffers[BUFFER_COUNT];
    void *mapped_ptrs[BUFFER_COUNT];
    uint32_t width;
    uint32_t height;
    uint32_t active_idx;
} bharat_lvgl_disp_ctx_t;

/* Stub IPC calls (assuming library-level stubs are present or injected) */
extern bh_display_result_t bh_client_create_surface(bh_display_lease_handle_t lease, uint32_t w, uint32_t h, uint32_t z, bh_gui_surface_handle_t *out_surf);
extern bh_display_result_t bh_client_register_buffer(bh_display_lease_handle_t lease, bh_display_buffer_desc_t *desc, bh_gui_buffer_handle_t *out_buf, void **out_mapped);
extern bh_display_result_t bh_client_attach_buffer(bh_display_lease_handle_t lease, bh_gui_surface_handle_t surf, bh_gui_buffer_handle_t buf);
extern bh_display_result_t bh_client_present_surface(bh_display_lease_handle_t lease, bh_gui_surface_handle_t surf, bh_gui_buffer_handle_t buf, bh_gui_fence_handle_t *out_release_fence);
extern bh_display_result_t bh_client_wait_fence(bh_gui_fence_handle_t fence, bh_monotonic_deadline_ns_t deadline);

static void bharat_lvgl_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    bharat_lvgl_disp_ctx_t *ctx = lv_display_get_user_data(disp);
    if (!ctx) {
        lv_display_flush_ready(disp);
        return;
    }

    /* For Phase 1 we support direct mapping or double buffering.
       If this is a partial flush, LVGL manages the dirty rects on the provided draw buffers. */

    // Present the buffer via the broker
    bh_gui_fence_handle_t release_fence = BH_GUI_HANDLE_INVALID;

    // Submitting current buffer
    bh_client_present_surface(ctx->lease, ctx->surface, ctx->buffers[ctx->active_idx], &release_fence);

    // In a fully asynchronous setup, we would wait on the release fence for the NEXT buffer
    // before calling flush_ready. For this P1 slice, we perform a blocking wait if a fence is returned.
    if (release_fence != BH_GUI_HANDLE_INVALID) {
        bh_client_wait_fence(release_fence, 1000000000ULL); // 1s timeout
    }

    /* Important: Inform LVGL that flushing is complete */
    lv_display_flush_ready(disp);
}

lv_display_t * bharat_lvgl_display_create(bh_display_lease_handle_t lease, uint32_t width, uint32_t height) {
    bharat_lvgl_disp_ctx_t *ctx = malloc(sizeof(bharat_lvgl_disp_ctx_t));
    if (!ctx) return NULL;

    memset(ctx, 0, sizeof(bharat_lvgl_disp_ctx_t));
    ctx->lease = lease;
    ctx->width = width;
    ctx->height = height;

    if (bh_client_create_surface(lease, width, height, 0, &ctx->surface) != BH_DISPLAY_RESULT_OK) {
        free(ctx);
        return NULL;
    }

    bh_display_buffer_desc_t desc;
    memset(&desc, 0, sizeof(desc));
    desc.abi_version = BH_GUI_HANDLE_ABI_V1;
    desc.struct_size = sizeof(desc);
    desc.width = width;
    desc.height = height;
    desc.pixel_format = BH_DISPLAY_FORMAT_XRGB8888;
    desc.usage_flags = BH_DISPLAY_BUFFER_USAGE_CPU_WRITE | BH_DISPLAY_BUFFER_USAGE_SCANOUT;
    desc.memory_domain = BH_DISPLAY_MEMORY_DOMAIN_SYSTEM;
    desc.plane_count = 1;
    desc.planes[0].stride_bytes = width * 4;
    desc.planes[0].size_bytes = width * height * 4;
    desc.total_size_bytes = desc.planes[0].size_bytes;

    for (int i = 0; i < BUFFER_COUNT; i++) {
        if (bh_client_register_buffer(lease, &desc, &ctx->buffers[i], &ctx->mapped_ptrs[i]) != BH_DISPLAY_RESULT_OK) {
            // Cleanup on failure omitted for brevity in P1 demo
            return NULL;
        }
        bh_client_attach_buffer(lease, ctx->surface, ctx->buffers[i]);
    }

    lv_display_t *disp = lv_display_create(width, height);
    lv_display_set_user_data(disp, ctx);
    lv_display_set_flush_cb(disp, bharat_lvgl_flush_cb);

    // Assign our capability-safe mapped buffers to LVGL
    lv_display_set_buffers(disp, ctx->mapped_ptrs[0], ctx->mapped_ptrs[1], desc.planes[0].size_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);

    return disp;
}
