/* SPDX-License-Identifier: MIT */
#include "ui_screens.h"

static lv_obj_t *progress_bar = NULL;
static lv_obj_t *stage_label = NULL;

bharat_ui_screen_t bharat_splash_screen_create(lv_obj_t *parent, const bharat_ui_context_t *context) {
    bharat_ui_screen_t screen;
    screen.id = BHARAT_SCREEN_SPLASH;

    screen.root = lv_obj_create(parent);
    lv_obj_set_size(screen.root, LV_PCT(100), LV_PCT(100));
    lv_obj_remove_flag(screen.root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(screen.root);
    lv_label_set_text(title, "Bharat-OS");
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -40);

    progress_bar = lv_bar_create(screen.root);
    lv_obj_set_size(progress_bar, 200, 20);
    lv_obj_align(progress_bar, LV_ALIGN_CENTER, 0, 20);
    lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);

    stage_label = lv_label_create(screen.root);
    lv_label_set_text(stage_label, "Initializing...");
    lv_obj_align(stage_label, LV_ALIGN_CENTER, 0, 60);

    return screen;
}

void bharat_splash_set_progress(bharat_ui_screen_t *screen, uint8_t percentage) {
    if (progress_bar) {
        if (percentage > 100) percentage = 100;
        lv_bar_set_value(progress_bar, percentage, LV_ANIM_ON);
    }
}

void bharat_splash_set_stage(bharat_ui_screen_t *screen, const char *stage) {
    if (stage_label && stage) {
        lv_label_set_text(stage_label, stage);
    }
}
