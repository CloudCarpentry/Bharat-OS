#include <assert.h>
#include <stdint.h>

#include "bharat/uapi/device/device_class.h"

int main(void) {
    /* Class identities remain stable for service contracts. */
    assert(BHARAT_DEV_CLASS_DISPLAY != BHARAT_DEV_CLASS_INPUT);
    assert(BHARAT_DEV_CLASS_MAX > BHARAT_DEV_CLASS_RADIO);

    /* Subclass registries exist for roadmap task P1. */
    assert(BHARAT_DISPLAY_SUBCLASS_PANEL > BHARAT_DISPLAY_SUBCLASS_UNKNOWN);
    assert(BHARAT_DISPLAY_SUBCLASS_VIDEO_OUT > BHARAT_DISPLAY_SUBCLASS_GPU);
    assert(BHARAT_INPUT_SUBCLASS_TOUCH > BHARAT_INPUT_SUBCLASS_UNKNOWN);
    assert(BHARAT_INPUT_SUBCLASS_CAN_CONTROL > BHARAT_INPUT_SUBCLASS_IR);

    /* Generic capability bits are distinct and composable. */
    assert((BHARAT_DISPLAY_CAP_OVERLAY & BHARAT_DISPLAY_CAP_DIRECT_SCANOUT) == 0);
    assert((BHARAT_INPUT_CAP_KEYS & BHARAT_INPUT_CAP_ROTARY_STEPS) == 0);

    /* Contracts must carry key fields from the UI execution roadmap. */
    bharat_display_caps_t display_caps = {
        .plane_count = 2,
        .pixel_format_count = 3,
        .capability_bits = BHARAT_DISPLAY_CAP_OVERLAY | BHARAT_DISPLAY_CAP_MODESET,
        .refresh_hz = {.min_hz = 48, .max_hz = 120},
        .max_mode = {.width_px = 1920, .height_px = 1080, .min_stride_bytes = 7680},
    };

    bharat_input_caps_t input_caps = {
        .source_id = 1,
        .coordinate_mode = 1,
        .keymap_set_id = 7,
        .rotary_resolution = 24,
        .capability_bits = BHARAT_INPUT_CAP_KEYS,
    };

    bharat_input_event_wire_t event = {
        .timestamp_ns = 1234567890ull,
        .type = 1,
        .code = 30,
        .value = 1,
    };

    assert(display_caps.max_mode.width_px == 1920);
    assert(display_caps.refresh_hz.max_hz == 120);
    assert(input_caps.keymap_set_id == 7);
    assert(event.timestamp_ns > 0);

    return 0;
}
