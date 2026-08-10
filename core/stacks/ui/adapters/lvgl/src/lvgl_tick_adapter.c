/* SPDX-License-Identifier: MIT */
#include "lvgl.h"
#include <stdint.h>
#include <time.h>

uint32_t bharat_lvgl_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

void bharat_lvgl_tick_init(void) {
    lv_tick_set_cb(bharat_lvgl_now_ms);
}
