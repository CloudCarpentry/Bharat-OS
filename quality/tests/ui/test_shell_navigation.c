/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "stubs/lvgl.h"
#include "bharat_shell.h"
#include "bharat/uapi/boot/boot_events.h"
#include "bharat/ui/theme.h"

static lv_obj_t mock_active_screen;
static lv_obj_t mock_objs[128];
static size_t mock_obj_idx = 0;
static lv_group_t mock_group;
static lv_timer_t mock_timer;
static bool mock_timer_created = false;
static bool mock_timer_deleted = false;

/* Mock theme matching bh_ui_theme_t */
static bh_ui_theme_t test_theme = {
    .brand_name = "BHARAT-OS",
    .tagline = "Capability Microkernel",
    .bg_color_rgb = 0x081426,
    .primary_color_rgb = 0xFF9933,
    .accent_color_rgb = 0x1E293B,
    .text_color_rgb = 0xF5F7FA,
    .text_dim_color_rgb = 0x94A3B8,
    .error_color_rgb = 0xEF4444,
    .show_spinner = true,
    .show_milestone_text = true,
};

const bh_ui_theme_t *bh_theme_get_active(void) {
    return &test_theme;
}

/* Mock LVGL APIs */
const int lv_font_montserrat_14 = 0;

lv_obj_t *lv_screen_active(void) { return &mock_active_screen; }
lv_obj_t *lv_obj_create(lv_obj_t *parent) { (void)parent; return &mock_objs[mock_obj_idx++ % 128]; }
lv_obj_t *lv_btn_create(lv_obj_t *parent) { (void)parent; return &mock_objs[mock_obj_idx++ % 128]; }
lv_obj_t *lv_label_create(lv_obj_t *parent) { (void)parent; return &mock_objs[mock_obj_idx++ % 128]; }
lv_obj_t *lv_spinner_create(lv_obj_t *parent) { (void)parent; return &mock_objs[mock_obj_idx++ % 128]; }

void lv_obj_set_size(lv_obj_t *obj, int32_t w, int32_t h) { (void)obj; (void)w; (void)h; }
void lv_obj_align(lv_obj_t *obj, int align, int32_t x, int32_t y) { (void)obj; (void)align; (void)x; (void)y; }
void lv_obj_center(lv_obj_t *obj) { (void)obj; }
void lv_obj_remove_flag(lv_obj_t *obj, int flag) { (void)obj; (void)flag; }
void lv_obj_delete(lv_obj_t *obj) { (void)obj; }

void lv_obj_set_style_bg_color(lv_obj_t *obj, lv_color_t color, lv_style_selector_t selector) { (void)obj; (void)color; (void)selector; }
void lv_obj_set_style_text_color(lv_obj_t *obj, lv_color_t color, lv_style_selector_t selector) { (void)obj; (void)color; (void)selector; }
void lv_obj_set_style_border_width(lv_obj_t *obj, int32_t w, lv_style_selector_t selector) { (void)obj; (void)w; (void)selector; }
void lv_obj_set_style_border_color(lv_obj_t *obj, lv_color_t color, lv_style_selector_t selector) { (void)obj; (void)color; (void)selector; }
void lv_obj_set_style_radius(lv_obj_t *obj, int32_t r, lv_style_selector_t selector) { (void)obj; (void)r; (void)selector; }
void lv_obj_set_style_shadow_width(lv_obj_t *obj, int32_t w, lv_style_selector_t selector) { (void)obj; (void)w; (void)selector; }
void lv_obj_set_style_outline_width(lv_obj_t *obj, int32_t w, lv_style_selector_t selector) { (void)obj; (void)w; (void)selector; }
void lv_obj_set_style_outline_color(lv_obj_t *obj, lv_color_t color, lv_style_selector_t selector) { (void)obj; (void)color; (void)selector; }
void lv_obj_set_style_outline_pad(lv_obj_t *obj, int32_t pad, lv_style_selector_t selector) { (void)obj; (void)pad; (void)selector; }
void lv_obj_set_style_bg_opa(lv_obj_t *obj, int opa, lv_style_selector_t selector) { (void)obj; (void)opa; (void)selector; }
void lv_obj_set_style_arc_color(lv_obj_t *obj, lv_color_t color, lv_style_selector_t selector) { (void)obj; (void)color; (void)selector; }
void lv_obj_set_style_text_font(lv_obj_t *obj, const void *font, lv_style_selector_t selector) { (void)obj; (void)font; (void)selector; }

void lv_obj_set_flex_flow(lv_obj_t *obj, int flow) { (void)obj; (void)flow; }
void lv_obj_set_flex_align(lv_obj_t *obj, int main, int cross, int track) { (void)obj; (void)main; (void)cross; (void)track; }
void lv_label_set_text(lv_obj_t *obj, const char *text) { (void)obj; (void)text; }
void lv_obj_add_event_cb(lv_obj_t *obj, void (*cb)(lv_event_t *), int event, void *user_data) { (void)obj; (void)cb; (void)event; (void)user_data; }
void *lv_event_get_user_data(lv_event_t *e) { (void)e; return NULL; }
uint32_t lv_event_get_key(lv_event_t *e) { (void)e; return 0; }

lv_color_t lv_color_hex(uint32_t hex) { return hex; }
int lv_snprintf(char *buf, size_t size, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int res = vsnprintf(buf, size, fmt, args);
    va_end(args);
    return res;
}

lv_group_t *lv_group_create(void) {
    memset(&mock_group, 0, sizeof(mock_group));
    return &mock_group;
}
void lv_group_add_obj(lv_group_t *group, lv_obj_t *obj) {
    if (group) group->obj_count++;
    (void)obj;
}
void lv_group_remove_all_objs(lv_group_t *group) {
    if (group) {
        group->obj_count = 0;
        group->focused = NULL;
    }
}
void lv_group_focus_obj(lv_obj_t *obj) {
    mock_group.focused = obj;
}
void lv_group_focus_next(lv_group_t *group) {
    if (group) group->focused = &mock_objs[0];
}

lv_timer_t *lv_timer_create(void (*timer_cb)(lv_timer_t *), uint32_t period, void *user_data) {
    (void)user_data;
    mock_timer.timer_cb = timer_cb;
    mock_timer.period = period;
    mock_timer.active = true;
    mock_timer_created = true;
    return &mock_timer;
}
void lv_timer_delete(lv_timer_t *timer) {
    if (timer) {
        timer->active = false;
        mock_timer_deleted = true;
    }
}

/* Mock boot events */
void bh_boot_events_get_snapshot(bh_boot_event_snapshot_t *out) {
    if (out) {
        memset(out, 0, sizeof(*out));
        out->count = 2;
        out->total_events = 2;
        out->events[0].stage = BH_BOOT_STAGE_HAL;
        out->events[0].status = BH_BOOT_STATUS_OK;
        strncpy(out->events[0].component, "HAL_INIT", sizeof(out->events[0].component) - 1);
        strncpy(out->events[0].message, "Hardware ready", sizeof(out->events[0].message) - 1);
        out->events[1].stage = BH_BOOT_STAGE_SERVICES;
        out->events[1].status = BH_BOOT_STATUS_OK;
        strncpy(out->events[1].component, "DISP_BROKER", sizeof(out->events[1].component) - 1);
        strncpy(out->events[1].message, "Display active", sizeof(out->events[1].message) - 1);
    }
}

const char *bh_boot_stage_name(bh_boot_stage_t stage) {
    (void)stage;
    return "TEST_STAGE";
}
const char *bh_boot_status_name(bh_boot_status_t status) {
    (void)status;
    return "OK";
}

/* Include shell UI implementation */
#include "../../experience/shell/shell_ui.c"

static void test_initialization_and_splash(void) {
    bh_shell_start();
    assert(bh_shell_current_screen() == BH_SHELL_SCREEN_SPLASH);
    assert(bh_shell_navigation_group() != NULL);
    printf("test_initialization_and_splash PASSED\n");
}

static void test_splash_to_launcher_transition(void) {
    bh_shell_start();
    assert(bh_shell_current_screen() == BH_SHELL_SCREEN_SPLASH);

    /* Transitioning to launcher sets Launcher as root (history reset) */
    int res = bh_shell_navigate(BH_SHELL_SCREEN_LAUNCHER);
    assert(res == 0);
    assert(bh_shell_current_screen() == BH_SHELL_SCREEN_LAUNCHER);

    /* Navigating back from root launcher must fail closed */
    res = bh_shell_navigate_back();
    assert(res == -1);
    assert(bh_shell_current_screen() == BH_SHELL_SCREEN_LAUNCHER);
    printf("test_splash_to_launcher_transition PASSED\n");
}

static void test_screen_navigation_and_back_stack(void) {
    bh_shell_start();
    (void)bh_shell_navigate(BH_SHELL_SCREEN_LAUNCHER);

    /* Navigate to System */
    mock_timer_created = false;
    mock_timer_deleted = false;
    assert(bh_shell_navigate(BH_SHELL_SCREEN_SYSTEM) == 0);
    assert(bh_shell_current_screen() == BH_SHELL_SCREEN_SYSTEM);
    assert(mock_timer_created == true);

    /* Navigate back to Launcher cleans up timer */
    assert(bh_shell_navigate_back() == 0);
    assert(bh_shell_current_screen() == BH_SHELL_SCREEN_LAUNCHER);
    assert(mock_timer_deleted == true);

    /* Navigate to Devices */
    assert(bh_shell_navigate(BH_SHELL_SCREEN_DEVICES) == 0);
    assert(bh_shell_current_screen() == BH_SHELL_SCREEN_DEVICES);

    /* Navigate to Diagnostics */
    assert(bh_shell_navigate(BH_SHELL_SCREEN_DIAGNOSTICS) == 0);
    assert(bh_shell_current_screen() == BH_SHELL_SCREEN_DIAGNOSTICS);

    /* Navigate to Demos */
    assert(bh_shell_navigate(BH_SHELL_SCREEN_DEMOS) == 0);
    assert(bh_shell_current_screen() == BH_SHELL_SCREEN_DEMOS);

    /* Pop all the way back to Launcher */
    assert(bh_shell_navigate_back() == 0);
    assert(bh_shell_current_screen() == BH_SHELL_SCREEN_DIAGNOSTICS);
    assert(bh_shell_navigate_back() == 0);
    assert(bh_shell_current_screen() == BH_SHELL_SCREEN_DEVICES);
    assert(bh_shell_navigate_back() == 0);
    assert(bh_shell_current_screen() == BH_SHELL_SCREEN_LAUNCHER);

    /* At root, further back is rejected */
    assert(bh_shell_navigate_back() == -1);
    printf("test_screen_navigation_and_back_stack PASSED\n");
}

static void test_reentrant_and_boundary_navigation(void) {
    bh_shell_start();
    (void)bh_shell_navigate(BH_SHELL_SCREEN_LAUNCHER);

    /* Navigating to same active screen is a no-op returning 0 */
    assert(bh_shell_navigate(BH_SHELL_SCREEN_LAUNCHER) == 0);
    assert(bh_shell_current_screen() == BH_SHELL_SCREEN_LAUNCHER);

    /* Invalid screen ID is rejected */
    assert(bh_shell_navigate(BH_SHELL_SCREEN_COUNT) == -1);
    assert(bh_shell_navigate((bh_shell_screen_id_t)999) == -1);
    assert(bh_shell_current_screen() == BH_SHELL_SCREEN_LAUNCHER);

    /* Exceeding max history depth */
    for (int i = 0; i < BH_SHELL_HISTORY_DEPTH; i++) {
        bh_shell_screen_id_t target = (i % 2 == 0) ? BH_SHELL_SCREEN_SYSTEM : BH_SHELL_SCREEN_DEVICES;
        assert(bh_shell_navigate(target) == 0);
    }
    /* Next navigation beyond depth 8 must fail cleanly */
    assert(bh_shell_navigate(BH_SHELL_SCREEN_DIAGNOSTICS) == -1);

    printf("test_reentrant_and_boundary_navigation PASSED\n");
}

int main(void) {
    printf("--- Running Bharat-OS Shell UI & Navigation Tests ---\n");
    test_initialization_and_splash();
    test_splash_to_launcher_transition();
    test_screen_navigation_and_back_stack();
    test_reentrant_and_boundary_navigation();
    printf("All Bharat-OS Shell UI tests PASSED successfully!\n");
    return 0;
}
