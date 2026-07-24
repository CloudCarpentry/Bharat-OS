#include <assert.h>
#include <stdint.h>

#include "bharat/ui/tiny_ui.h"

int main(void) {
    static uint32_t rgb_fb[64u * 48u];
    static uint8_t mono_fb[64u * 48u];

    bharat_tiny_ui_state_t state;
    bharat_tiny_ui_init(&state, false);
    assert(state.page == BHARAT_UI_PAGE_SPLASH);
    assert(state.progress_percent == 0u);

    for (int i = 0; i < 4; ++i) {
        bharat_tiny_ui_apply_input(&state, BHARAT_UI_INPUT_SELECT);
    }
    assert(state.progress_percent == 40u);

    bharat_tiny_ui_apply_input(&state, BHARAT_UI_INPUT_NEXT);
    assert(state.page == BHARAT_UI_PAGE_DIAGNOSTICS);

    bharat_tiny_fb_t rgb = {
        .width_px = 64,
        .height_px = 48,
        .stride_bytes = 64u * (uint32_t)sizeof(uint32_t),
        .pixel_format = BHARAT_UI_PIXEL_FMT_XRGB8888,
        .pixels = rgb_fb,
    };
    bharat_tiny_ui_render(&rgb, &state);
    assert(rgb_fb[0] != 0u);

    bharat_tiny_ui_init(&state, true);
    bharat_tiny_fb_t mono = {
        .width_px = 64,
        .height_px = 48,
        .stride_bytes = 64u,
        .pixel_format = BHARAT_UI_PIXEL_FMT_MONO8,
        .pixels = mono_fb,
    };
    bharat_tiny_ui_render(&mono, &state);
    assert(mono_fb[0] == 0xFFu);

    return 0;
}
