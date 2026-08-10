/* SPDX-License-Identifier: MIT */
#ifndef BHARAT_LVGL_H
#define BHARAT_LVGL_H

#include <stdint.h>

uint32_t bharat_lvgl_now_ms(void);
void bharat_lvgl_wait_ms(uint32_t delay_ms);
void bharat_lvgl_tick_init(void);

#endif /* BHARAT_LVGL_H */
