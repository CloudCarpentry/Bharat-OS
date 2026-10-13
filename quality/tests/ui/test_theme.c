/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "bharat/ui/theme.h"

static void test_default_theme(void) {
    bh_theme_reset_default();
    const bh_ui_theme_t *theme = bh_theme_get_active();
    assert(theme != NULL);
    assert(strcmp(theme->brand_name, "BHARAT-OS") == 0);
    assert(theme->primary_color_rgb == 0xFF9933);
    assert(theme->bg_color_rgb == 0x081426);
    assert(theme->show_spinner == true);
    assert(theme->show_milestone_text == true);
    printf("test_default_theme PASSED\n");
}

static void test_custom_theme_valid(void) {
    bh_theme_reset_default();

    bh_ui_theme_t oem_theme = {
        .brand_name = "AutoBharat OEM",
        .tagline = "Connected Automotive Platform",
        .bg_color_rgb = 0x111111,
        .primary_color_rgb = 0x00FF88,
        .accent_color_rgb = 0x0088FF,
        .text_color_rgb = 0xFFFFFF,
        .text_dim_color_rgb = 0x888888,
        .error_color_rgb = 0xFF0000,
        .show_spinner = false,
        .show_milestone_text = true,
        .logo_bitmap = NULL,
        .logo_width = 0,
        .logo_height = 0
    };

    int rc = bh_theme_set_custom(&oem_theme);
    assert(rc == 0);

    const bh_ui_theme_t *active = bh_theme_get_active();
    assert(strcmp(active->brand_name, "AutoBharat OEM") == 0);
    assert(strcmp(active->tagline, "Connected Automotive Platform") == 0);
    assert(active->primary_color_rgb == 0x00FF88);
    assert(active->bg_color_rgb == 0x111111);
    assert(active->show_spinner == false);

    bh_theme_reset_default();
    assert(strcmp(bh_theme_get_active()->brand_name, "BHARAT-OS") == 0);
    printf("test_custom_theme_valid PASSED\n");
}

static void test_custom_theme_invalid(void) {
    bh_theme_reset_default();

    /* Null theme pointer */
    assert(bh_theme_set_custom(NULL) == -1);

    /* Empty brand name */
    bh_ui_theme_t invalid_name = {
        .brand_name = "",
        .primary_color_rgb = 0xFF9933
    };
    assert(bh_theme_set_custom(&invalid_name) == -2);

    /* Logo bitmap provided without dimensions */
    uint8_t dummy_logo[16] = {0};
    bh_ui_theme_t invalid_logo = {
        .brand_name = "Valid Name",
        .logo_bitmap = dummy_logo,
        .logo_width = 0,
        .logo_height = 0
    };
    assert(bh_theme_set_custom(&invalid_logo) == -3);

    printf("test_custom_theme_invalid PASSED\n");
}

int main(void) {
    test_default_theme();
    test_custom_theme_valid();
    test_custom_theme_invalid();

    printf("All UI theme tests PASSED!\n");
    return 0;
}
