#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Mock printf
int printf(const char *format, ...) {
    (void)format;
    return 0;
}

// User space minimal boot_display service.
// This service would be invoked with capabilities passed via IPC or env,
// rendering a simple splash screen and progress bar using the mapped framebuffer.

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t *pixels;
} fb_surface_t;

void draw_rect(fb_surface_t *fb, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    if (x >= fb->width || y >= fb->height) return;

    if (x + w > fb->width) w = fb->width - x;
    if (y + h > fb->height) h = fb->height - y;

    for (uint32_t row = y; row < y + h; row++) {
        uint32_t *row_ptr = (uint32_t*)((uint8_t*)fb->pixels + (row * fb->stride));
        for (uint32_t col = x; col < x + w; col++) {
            row_ptr[col] = color;
        }
    }
}

int main(int argc, char **argv) {
    printf("[boot_displayd] Starting early boot UI...\n");

    // In a real system, we'd request the framebuffer memory mapped capability
    // capability_t fb_cap = get_capability("CAP_DISPLAY_FB");
    // void* fb_mem = map_capability(fb_cap);

    // Mock testing
    fb_surface_t mock_fb = {
        .width = 1024,
        .height = 768,
        .stride = 1024 * 4,
        .pixels = NULL // normally mapped vaddr
    };

    if (mock_fb.pixels) {
        // Draw background
        draw_rect(&mock_fb, 0, 0, mock_fb.width, mock_fb.height, 0xFF222222);

        // Draw progress bar
        uint32_t pb_w = 400;
        uint32_t pb_h = 20;
        uint32_t pb_x = (mock_fb.width - pb_w) / 2;
        uint32_t pb_y = mock_fb.height - 100;

        draw_rect(&mock_fb, pb_x, pb_y, pb_w, pb_h, 0xFF444444);
        draw_rect(&mock_fb, pb_x, pb_y, pb_w / 3, pb_h, 0xFF00FF00); // 33% progress
    }

    printf("[boot_displayd] Early UI rendered. Waiting for display handoff...\n");

    // Loop waiting for compositor or displayd takeover signals
    while (true) {
        // sleep or wait for message
        break;
    }

    printf("[boot_displayd] Handoff complete. Exiting.\n");
    return 0;
}
