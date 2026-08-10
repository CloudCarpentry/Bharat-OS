/* SPDX-License-Identifier: MIT */
#include "lvgl.h"
#include "bharat/uapi/display/display_v2.h"
#include "bharat/uapi/display/bharat_display_broker_v2_types.h"
#include "bharat_lvgl.h"
#include "ui_screens.h"
#include <stdio.h>
#include <stdlib.h>

// Forward declarations for adapters
extern lv_display_t * bharat_lvgl_display_create(bh_display_lease_handle_t lease, uint32_t width, uint32_t height);
extern lv_indev_t * bharat_lvgl_pointer_create(void);
extern lv_indev_t * bharat_lvgl_keyboard_create(void);
extern void bharat_ui_app_start(void);

// Simulated IPC stubs for display broker logic so it builds/links smoothly
bh_display_result_t bh_client_create_surface(bh_display_lease_handle_t lease, uint32_t w, uint32_t h, uint32_t z, bh_gui_surface_handle_t *out_surf) {
    (void)lease; (void)w; (void)h; (void)z;
    *out_surf = 42; // Dummy handle
    return BH_DISPLAY_RESULT_OK;
}
bh_display_result_t bh_client_register_buffer(bh_display_lease_handle_t lease, bh_display_buffer_desc_t *desc, bh_gui_buffer_handle_t *out_buf, void **out_mapped) {
    (void)lease;
    *out_buf = 43; // Dummy handle
    *out_mapped = malloc(desc->planes[0].size_bytes);
    return BH_DISPLAY_RESULT_OK;
}
bh_display_result_t bh_client_attach_buffer(bh_display_lease_handle_t lease, bh_gui_surface_handle_t surf, bh_gui_buffer_handle_t buf) {
    (void)lease; (void)surf; (void)buf;
    return BH_DISPLAY_RESULT_OK;
}
bh_display_result_t bh_client_present_surface(bh_display_lease_handle_t lease, bh_gui_surface_handle_t surf, bh_gui_buffer_handle_t buf, bh_gui_fence_handle_t *out_release_fence) {
    (void)lease; (void)surf; (void)buf;
    *out_release_fence = BH_GUI_HANDLE_INVALID;
    return BH_DISPLAY_RESULT_OK;
}
bh_display_result_t bh_client_wait_fence(bh_gui_fence_handle_t fence, bh_monotonic_deadline_ns_t deadline) {
    (void)fence; (void)deadline;
    return BH_DISPLAY_RESULT_OK;
}
int bh_inputmgr_drain(void *out_events, int max_events) {
    (void)out_events; (void)max_events;
    return 0; // Return 0 events normally
}

int main(int argc, char** argv) {
    printf("UI_NATIVE: START\n");

    /* Initialize LVGL */
    lv_init();
    bharat_lvgl_tick_init();
    printf("UI_NATIVE: LVGL_READY\n");

    /* Mock leasing a display (ID 0) from the display broker */
    bh_display_lease_handle_t default_lease = 1;

    /* Create a native LVGL display adapter wrapping the broker buffers */
    lv_display_t * disp = bharat_lvgl_display_create(default_lease, 1024, 768);
    if (!disp) {
        printf("UI_NATIVE: DISPLAY_UNAVAILABLE\n");
        return -1;
    }
    printf("UI_NATIVE: DISPLAY_CONNECTED\n");

    /* Create input devices mapped to our input manager */
    lv_indev_t * pointer = bharat_lvgl_pointer_create();
    if(pointer) {
        printf("UI_NATIVE: POINTER_READY\n");
    } else {
        printf("UI_NATIVE: INPUT_DEGRADED\n");
    }

    lv_indev_t * keyboard = bharat_lvgl_keyboard_create();
    if(keyboard) {
        printf("UI_NATIVE: KEYBOARD_READY\n");
    } else {
        printf("UI_NATIVE: INPUT_DEGRADED\n");
    }

    /* Transition to branded splash screen */
    bharat_ui_app_start();

    /* Ensure the screen is actually rendered */
    lv_timer_handler();
    printf("UI_NATIVE: SPLASH_VISIBLE\n");

    /* In a real environment, wait briefly, then transition to HOME.
       For this showcase we manually advance. */
    // Note: bharat_ui_app_start initializes with Splash.
    extern int bharat_ui_navigate(int target);
    bharat_ui_navigate(1 /* BHARAT_SCREEN_HOME */);
    lv_timer_handler();
    printf("UI_NATIVE: HOME_VISIBLE\n");

    /* Main UI Pump loop */
    int frame_count = 0;
    while(frame_count < 10) { // Limit iterations for demo test run
        uint32_t delay = lv_timer_handler();
        bharat_lvgl_wait_ms(delay);
        frame_count++;
    }

    return 0;
}
