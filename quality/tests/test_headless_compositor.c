#include <assert.h>
#include <stdio.h>
#include "core/stacks/media/compositor/backends/headless/headless_backend.h"

int main(void) {
    printf("Running test_headless_compositor...\n");

    bharat_headless_compositor_t comp;
    bharat_headless_compositor_init(&comp);

    uint64_t s1_id;
    assert(bharat_headless_compositor_create_surface(&comp, 800, 600, &s1_id) == 0);
    assert(s1_id == 1);
    assert(comp.surface_count == 1);

    assert(bharat_headless_compositor_submit_buffer(&comp, s1_id, 100) == 0);
    assert(comp.active_buffers[0] == 100);

    bharat_headless_compositor_present(&comp);
    assert(comp.frame_counter == 1);

    printf("test_headless_compositor passed!\n");
    return 0;
}
