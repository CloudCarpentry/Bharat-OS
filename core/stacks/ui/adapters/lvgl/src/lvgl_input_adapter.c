/* SPDX-License-Identifier: MIT */
#include "lvgl.h"
#include "bharat/uapi/input/input_event.h"

// Define a stub structure or use the actual API if the input manager is globally available.
// The actual showcase application will populate events into these buffers or pump them directly.
extern int bh_inputmgr_drain(bh_input_event_t *out_events, int max_events);

static int32_t cursor_x = 0;
static int32_t cursor_y = 0;
static bool left_button_down = false;
static uint32_t last_key = 0;
static lv_indev_state_t last_key_state = LV_INDEV_STATE_RELEASED;

// Read callback for LVGL Pointer (Mouse/Touch)
static void bharat_lvgl_pointer_read_cb(lv_indev_t * indev, lv_indev_data_t * data) {
    // In a full implementation, we pull from a lock-free ring buffer or rely on the main loop
    // to feed state changes into this adapter.

    // Drain events from the input manager
    bh_input_event_t events[16];
    int count = bh_inputmgr_drain(events, 16);

    for (int i = 0; i < count; i++) {
        if (events[i].type == 2) { // BHARAT_INPUT_POINTER / EV_REL
            if (events[i].code == 0) { // REL_X
                cursor_x += events[i].value;
            } else if (events[i].code == 1) { // REL_Y
                cursor_y += events[i].value;
            }
        } else if (events[i].type == 1) { // BHARAT_INPUT_KEY / EV_KEY
            if (events[i].code == 272) { // BTN_LEFT
                left_button_down = (events[i].value != 0);
            }
        }
    }

    // Clamp coordinates (using generic bounds for now)
    lv_display_t * disp = lv_indev_get_display(indev);
    if (disp) {
        int32_t max_x = lv_display_get_horizontal_resolution(disp) - 1;
        int32_t max_y = lv_display_get_vertical_resolution(disp) - 1;
        if (cursor_x < 0) cursor_x = 0;
        if (cursor_x > max_x) cursor_x = max_x;
        if (cursor_y < 0) cursor_y = 0;
        if (cursor_y > max_y) cursor_y = max_y;
    }

    data->point.x = cursor_x;
    data->point.y = cursor_y;
    data->state = left_button_down ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

// Read callback for LVGL Keyboard
static void bharat_lvgl_keyboard_read_cb(lv_indev_t * indev, lv_indev_data_t * data) {
    data->key = last_key;
    data->state = last_key_state;
    // Consume key
    if(last_key_state == LV_INDEV_STATE_RELEASED) {
        last_key = 0;
    }
}

// Global handle to inject key events from showcase app
void bharat_lvgl_inject_key(uint32_t key, bool pressed) {
    last_key = key;
    last_key_state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

lv_indev_t * bharat_lvgl_pointer_create(void) {
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, bharat_lvgl_pointer_read_cb);
    return indev;
}

lv_indev_t * bharat_lvgl_keyboard_create(void) {
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(indev, bharat_lvgl_keyboard_read_cb);
    return indev;
}
