#include <stdbool.h>
#include <stdint.h>

#include "bharat/ui/tiny_ui.h"
#include "bharat/uapi/display/boot_display.h"

static bharat_ui_input_action_t boot_displayd_map_mock_input(unsigned tick) {
    if (tick % 5u == 0u) {
        return BHARAT_UI_INPUT_NEXT;
    }
    return BHARAT_UI_INPUT_SELECT;
}

int main(void) {
    bharat_boot_display_state_t current_state = BHARAT_BOOT_DISPLAY_STATE_SPLASH_ALLOWED;
    (void)current_state;

    static uint32_t mock_fb_memory[800u * 480u];

    bharat_tiny_fb_t fb = {
        .width_px = 800,
        .height_px = 480,
        .stride_bytes = 800u * (uint32_t)sizeof(uint32_t),
        .pixel_format = BHARAT_UI_PIXEL_FMT_XRGB8888,
        .pixels = mock_fb_memory,
    };

    bharat_tiny_ui_state_t ui_state;
    bharat_tiny_ui_init(&ui_state, false);

    for (unsigned tick = 0; tick < 8u; ++tick) {
        bharat_ui_input_action_t action = boot_displayd_map_mock_input(tick);
        bharat_tiny_ui_apply_input(&ui_state, action);
        bharat_tiny_ui_render(&fb, &ui_state);
    }

    return 0;
}
