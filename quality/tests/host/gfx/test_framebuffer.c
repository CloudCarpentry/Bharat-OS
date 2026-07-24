#include <assert.h>
#include <stdio.h>
#include "core/lib/gfx/framebuffer.h"

int main(void) {
    printf("Running test_framebuffer...\n");

    /* Test min stride calculation */
    assert(bharat_gfx_framebuffer_min_stride(1920, BHARAT_PIXFMT_XRGB8888) == 1920 * 4);
    assert(bharat_gfx_framebuffer_min_stride(1920, BHARAT_PIXFMT_RGB565) == 1920 * 2);

    /* Test min size calculation */
    assert(bharat_gfx_framebuffer_min_size(100, 100, BHARAT_PIXFMT_XRGB8888) == 100 * 100 * 4);

    /* Test validation */
    bharat_framebuffer_t fb = {
        .width = 1920,
        .height = 1080,
        .pixel_format = BHARAT_PIXFMT_XRGB8888,
        .stride_bytes = 1920 * 4,
        .pixels = NULL,
        .buffer_id = 1
    };

    assert(bharat_gfx_framebuffer_is_valid(&fb) == true);

    fb.width = 0;
    assert(bharat_gfx_framebuffer_is_valid(&fb) == false);

    fb.width = 1920;
    fb.stride_bytes = 10;
    assert(bharat_gfx_framebuffer_is_valid(&fb) == false);

    printf("test_framebuffer passed!\n");
    return 0;
}
