/* SPDX-License-Identifier: Zlib */
#include "lvgl.h"
#include <stdio.h>
#include <stdbool.h>

extern void bharat_ui_app_start(void);

#define BHARAT_SIM_WIDTH 800
#define BHARAT_SIM_HEIGHT 480

static bool simulator_running = true;

bool bharat_ui_simulator_running(void) {
    return simulator_running;
}

int main(int argc, char** argv) {
    printf("Starting Bharat-OS UI Simulator (Host)...\n");

    lv_init();

    lv_display_t *display = lv_sdl_window_create(BHARAT_SIM_WIDTH, BHARAT_SIM_HEIGHT);
    if (!display) {
        printf("Failed to create SDL window.\n");
        return -1;
    }

    lv_indev_t *mouse = lv_sdl_mouse_create();
    lv_indev_t *wheel = lv_sdl_mousewheel_create();
    lv_indev_t *keyboard = lv_sdl_keyboard_create();

    bharat_ui_app_start();

    while (bharat_ui_simulator_running()) {
        uint32_t delay_ms = lv_timer_handler();
        if (delay_ms < 1u) {
            delay_ms = 1u;
        }
        if (delay_ms > 10u) {
            delay_ms = 10u;
        }

        lv_delay_ms(delay_ms);
    }

    lv_deinit();
    return 0;
}
