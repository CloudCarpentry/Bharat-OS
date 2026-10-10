/* SPDX-License-Identifier: MIT */
#include "bharat/ui/theme.h"

static const bh_ui_theme_t g_default_bharat_theme = {
    .brand_name = "BHARAT-OS",
    .tagline = "One core vision. Many architectures.",
    .bg_color_rgb = 0x081426,       /* Navy blue deep background */
    .primary_color_rgb = 0xFF9933,  /* Saffron brand color */
    .accent_color_rgb = 0x138808,   /* India green accent */
    .text_color_rgb = 0xF5F7FA,     /* Crisp white text */
    .text_dim_color_rgb = 0x9E9E9E, /* Subdued gray */
    .error_color_rgb = 0xEF4444,    /* Amber red */
    .show_spinner = true,
    .show_milestone_text = true,
    .logo_bitmap = NULL,
    .logo_width = 0,
    .logo_height = 0
};

static bh_ui_theme_t g_active_theme = {
    .brand_name = "BHARAT-OS",
    .tagline = "One core vision. Many architectures.",
    .bg_color_rgb = 0x081426,
    .primary_color_rgb = 0xFF9933,
    .accent_color_rgb = 0x138808,
    .text_color_rgb = 0xF5F7FA,
    .text_dim_color_rgb = 0x9E9E9E,
    .error_color_rgb = 0xEF4444,
    .show_spinner = true,
    .show_milestone_text = true,
    .logo_bitmap = NULL,
    .logo_width = 0,
    .logo_height = 0
};

const bh_ui_theme_t *bh_theme_get_active(void) {
    return &g_active_theme;
}

int bh_theme_set_custom(const bh_ui_theme_t *custom_theme) {
    if (!custom_theme) {
        return -1;
    }

    if (!custom_theme->brand_name || custom_theme->brand_name[0] == '\0') {
        return -2; /* Brand name required */
    }

    if (custom_theme->logo_bitmap != NULL && (custom_theme->logo_width == 0 || custom_theme->logo_height == 0)) {
        return -3; /* Invalid dimensions for provided logo bitmap */
    }

    g_active_theme = *custom_theme;
    return 0;
}

void bh_theme_reset_default(void) {
    g_active_theme = g_default_bharat_theme;
}
