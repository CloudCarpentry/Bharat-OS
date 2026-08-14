#include <stdbool.h>
#include <stdint.h>

#include "bharat/ui/tiny_ui.h"

#define UI_ASSERT(cond) do { if (!(cond)) { return 1; } } while(0)

int main(void) {
    static uint32_t rgb_fb[64u * 48u];
    static uint8_t mono_fb[64u * 48u];

    bharat_tiny_ui_state_t state;
    bharat_tiny_ui_init(&state, false);
    UI_ASSERT(state.page == BHARAT_UI_PAGE_SPLASH);
    UI_ASSERT(state.progress_percent == 0u);

    for (int i = 0; i < 4; ++i) {
        bharat_tiny_ui_apply_input(&state, BHARAT_UI_INPUT_SELECT);
    }
    UI_ASSERT(state.progress_percent == 40u);

    bharat_tiny_ui_apply_input(&state, BHARAT_UI_INPUT_NEXT);
    UI_ASSERT(state.page == BHARAT_UI_PAGE_DIAGNOSTICS);

    bharat_tiny_fb_t rgb = {
        .width_px = 64,
        .height_px = 48,
        .stride_bytes = 64u * (uint32_t)sizeof(uint32_t),
        .pixel_format = BHARAT_UI_PIXEL_FMT_XRGB8888,
        .pixels = rgb_fb,
    };
    bharat_tiny_ui_render(&rgb, &state);
    UI_ASSERT(rgb_fb[0] != 0u);

    bharat_tiny_ui_init(&state, true);
    bharat_tiny_fb_t mono = {
        .width_px = 64,
        .height_px = 48,
        .stride_bytes = 64u,
        .pixel_format = BHARAT_UI_PIXEL_FMT_MONO8,
        .pixels = mono_fb,
    };
    bharat_tiny_ui_render(&mono, &state);
    UI_ASSERT(mono_fb[0] == 0xFFu);

    /* Test rich text and outline drawing functions */
    bharat_tiny_ui_draw_rect(&rgb, 0, 0, 64, 48, 0xFF000000u);
    bharat_tiny_ui_draw_rect_outline(&rgb, 2, 2, 60, 44, 0xFFFFFFFFu);
    bharat_tiny_ui_draw_text(&rgb, 4, 4, "BHARAT", 0xFFFF9900u, 0xFF000000u, 1u);
    UI_ASSERT(rgb_fb[0] == 0xFF000000u);

    return 0;
}
