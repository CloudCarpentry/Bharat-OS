#ifndef BHARAT_COMPOSITOR_HEADLESS_H
#define BHARAT_COMPOSITOR_HEADLESS_H

#include <stdint.h>
#include <stdbool.h>
#include "bharat/uapi/gui/surface.h"

#define MAX_SURFACES 16

typedef struct {
    bharat_gui_surface_t surfaces[MAX_SURFACES];
    uint64_t active_buffers[MAX_SURFACES];
    uint32_t surface_count;
    uint64_t frame_counter;
} bharat_headless_compositor_t;

void bharat_headless_compositor_init(bharat_headless_compositor_t *ctx);
int32_t bharat_headless_compositor_create_surface(bharat_headless_compositor_t *ctx, uint32_t w, uint32_t h, uint64_t *id);
int32_t bharat_headless_compositor_submit_buffer(bharat_headless_compositor_t *ctx, uint64_t surface_id, uint64_t buffer_id);
void bharat_headless_compositor_present(bharat_headless_compositor_t *ctx);

#endif /* BHARAT_COMPOSITOR_HEADLESS_H */
