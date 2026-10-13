/* SPDX-License-Identifier: MIT */
#ifndef BHARAT_UI_THEME_H
#define BHARAT_UI_THEME_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *brand_name;
    const char *tagline;
    uint32_t bg_color_rgb;
    uint32_t primary_color_rgb;
    uint32_t accent_color_rgb;
    uint32_t text_color_rgb;
    uint32_t text_dim_color_rgb;
    uint32_t error_color_rgb;
    bool show_spinner;
    bool show_milestone_text;
    const uint8_t *logo_bitmap;
    uint16_t logo_width;
    uint16_t logo_height;
} bh_ui_theme_t;

/* Retrieve active system theme */
const bh_ui_theme_t *bh_theme_get_active(void);

/* Set custom OEM theme; returns 0 on success, negative error on invalid configuration */
int bh_theme_set_custom(const bh_ui_theme_t *custom_theme);

/* Reset active theme back to official Bharat-OS defaults */
void bh_theme_reset_default(void);

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_UI_THEME_H */
