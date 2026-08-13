/* SPDX-License-Identifier: MIT */
#include "bharat/uapi/display/bharat_display_broker_v2_types.h"
#include "bharat/uapi/display/display_v2.h"
#include "bharat_lvgl.h"
#include "bharat_shell.h"
#include "display_client.h"
#include "lvgl.h"
#include <stdio.h>

// Forward declarations for adapters
extern lv_display_t *bharat_lvgl_display_create(bh_display_lease_handle_t lease,
                                                uint32_t width,
                                                uint32_t height);
extern lv_indev_t *bharat_lvgl_pointer_create(void);
extern lv_indev_t *bharat_lvgl_keyboard_create(void);
static void demo_snapshot(bh_shell_system_info_t *info, void *context) {
  (void)context;
  info->uptime_seconds = bharat_lvgl_now_ms() / 1000U;
}

int bh_inputmgr_drain(void *out_events, int max_events) {
  (void)out_events;
  (void)max_events;
  return 0; // Return 0 events normally
}

int main(int argc, char **argv) {
  bh_showcase_display_session_t display_session;
  bh_display_result_t display_result;

  (void)argc;
  (void)argv;
  printf("UI_NATIVE: START\n");

  /* Initialize LVGL */
  lv_init();
  bharat_lvgl_tick_init();
  printf("UI_NATIVE: LVGL_READY\n");

  display_result = bh_showcase_display_open(&display_session);
  if (display_result != BH_DISPLAY_RESULT_OK) {
    printf("UI_NATIVE: DISPLAY_UNAVAILABLE result=%u\n",
           (unsigned)display_result);
    return -1;
  }
  printf("[gui] display-ready width=%u height=%u refresh=%u\n",
         display_session.width, display_session.height,
         display_session.refresh_hz);

  /* Create a native LVGL display adapter wrapping the broker buffers */
  lv_display_t *disp = bharat_lvgl_display_create(
      display_session.lease, display_session.width, display_session.height);
  if (!disp) {
    printf("UI_NATIVE: DISPLAY_UNAVAILABLE\n");
    return -1;
  }
  printf("UI_NATIVE: DISPLAY_CONNECTED\n");

  /* Create input devices mapped to our input manager */
  lv_indev_t *pointer = bharat_lvgl_pointer_create();
  if (pointer) {
    printf("UI_NATIVE: POINTER_READY\n");
  } else {
    printf("UI_NATIVE: INPUT_DEGRADED\n");
  }

  lv_indev_t *keyboard = bharat_lvgl_keyboard_create();
  if (keyboard) {
    printf("UI_NATIVE: KEYBOARD_READY\n");
  } else {
    printf("UI_NATIVE: INPUT_DEGRADED\n");
  }

  /* Transition to branded splash screen */
  bh_shell_set_snapshot_provider(demo_snapshot, NULL);
  bh_shell_start();

  /* Ensure the screen is actually rendered */
  lv_timer_handler();
  printf("UI_NATIVE: SPLASH_VISIBLE\n");

  /* In a real environment, wait briefly, then transition to HOME.
     For this showcase we manually advance. */
  // Note: bharat_ui_app_start initializes with Splash.
  bh_shell_navigate(BH_SHELL_SCREEN_LAUNCHER);
  if (keyboard != NULL) {
    lv_indev_set_group(keyboard, bh_shell_navigation_group());
  }
  lv_timer_handler();
  printf("UI_NATIVE: HOME_VISIBLE\n");

  /* Main UI Pump loop */
  int frame_count = 0;
  while (frame_count < 10) { // Limit iterations for demo test run
    uint32_t delay = lv_timer_handler();
    bharat_lvgl_wait_ms(delay);
    frame_count++;
  }

  return 0;
}
