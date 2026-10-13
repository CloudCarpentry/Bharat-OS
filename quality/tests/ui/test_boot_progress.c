#include <bharat/ui/fbui_widgets.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>

static uint32_t fill_width;

void fbui_render_fill_rect(fbui_render_context_t *ctx, uint32_t x, uint32_t y,
                           uint32_t width, uint32_t height, uint32_t color) {
    (void)ctx; (void)x; (void)y; (void)height;
    if (color == 0xFF00FF00U) fill_width = width;
}

int main(void) {
    const uint32_t values[] = {0, 50, 100, UINT_MAX};
    const uint32_t expected[] = {0, 50, 100, 100};
    fbui_render_context_t ctx = {0};
    for (unsigned i = 0; i < 4; ++i) {
        fbui_widget_t *w = fbui_create_progress_percent(0, 0, 104, 16, values[i]);
        assert(w);
        fill_width = 0;
        w->ops->draw(w, &ctx);
        assert(fill_width == expected[i]);
    }
    /* Preserve the existing floating-point API for userspace callers. */
    fbui_widget_t *legacy = fbui_create_progress(0, 0, 104, 16, 0.25f);
    assert(legacy);
    fill_width = 0;
    legacy->ops->draw(legacy, &ctx);
    assert(fill_width == 25);
    assert(fbui_create_progress_percent(0, 0, 104, 16, 100) == NULL);
    puts("PASS: integer boot progress, clamping, legacy fraction, pool bounds");
}
