/* SPDX-License-Identifier: MIT */
#include "ui_screens.h"

extern int bharat_ui_navigate_back(void);

static void back_btn_event_cb(lv_event_t * e) {
    bharat_ui_navigate_back();
}

bharat_ui_screen_t bharat_settings_screen_create(lv_obj_t *parent, const bharat_ui_context_t *context) {
    bharat_ui_screen_t screen;
    screen.id = BHARAT_SCREEN_SETTINGS;

    screen.root = lv_obj_create(parent);
    lv_obj_set_size(screen.root, LV_PCT(100), LV_PCT(100));

    lv_obj_t *title = lv_label_create(screen.root);
    lv_label_set_text(title, "Settings");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *dd = lv_dropdown_create(screen.root);
    lv_dropdown_set_options(dd, "Light Theme\nDark Theme");
    lv_obj_align(dd, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t *back_btn = lv_btn_create(screen.root);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_btn_label = lv_label_create(back_btn);
    lv_label_set_text(back_btn_label, "Back");

    return screen;
}
