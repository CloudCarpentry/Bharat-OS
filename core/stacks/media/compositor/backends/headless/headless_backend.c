#include <lib/base/string.h>
#include "headless_backend.h"

void bharat_headless_compositor_init(bharat_headless_compositor_t *ctx) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));
}

int32_t bharat_headless_compositor_create_surface(bharat_headless_compositor_t *ctx, uint32_t w, uint32_t h, uint64_t *id) {
    if (ctx->surface_count >= MAX_SURFACES) return -1;

    uint32_t idx = ctx->surface_count++;
    ctx->surfaces[idx].surface_id = idx + 1;
    ctx->surfaces[idx].width = w;
    ctx->surfaces[idx].height = h;
    ctx->surfaces[idx].state = BHARAT_SURFACE_STATE_CREATED;

    if (id) *id = ctx->surfaces[idx].surface_id;
    return 0;
}

int32_t bharat_headless_compositor_submit_buffer(bharat_headless_compositor_t *ctx, uint64_t surface_id, uint64_t buffer_id) {
    if (surface_id == 0 || surface_id > MAX_SURFACES) return -1;

    ctx->active_buffers[surface_id - 1] = buffer_id;
    return 0;
}

void bharat_headless_compositor_present(bharat_headless_compositor_t *ctx) {
    ctx->frame_counter++;
}
