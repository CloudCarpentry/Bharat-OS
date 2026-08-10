/* SPDX-License-Identifier: MIT */
#include "ui_screens.h"

extern int bharat_ui_navigate_back(void);

static void back_btn_event_cb(lv_event_t * e) {
    bharat_ui_navigate_back();
}

bharat_ui_screen_t bharat_diagnostics_screen_create(lv_obj_t *parent, const bharat_ui_context_t *context) {
    bharat_ui_screen_t screen;
    screen.id = BHARAT_SCREEN_DIAGNOSTICS;

    screen.root = lv_obj_create(parent);
    lv_obj_set_size(screen.root, LV_PCT(100), LV_PCT(100));

    lv_obj_t *title = lv_label_create(screen.root);
    lv_label_set_text(title, "Diagnostics");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *list = lv_list_create(screen.root);
    lv_obj_set_size(list, LV_PCT(90), LV_PCT(70));
    lv_obj_align(list, LV_ALIGN_CENTER, 0, 10);

    lv_list_add_text(list, "Boot Stage");
    lv_list_add_btn(list, NULL, "Kernel: Loaded");
    lv_list_add_btn(list, NULL, "Drivers: Initialized");
    lv_list_add_btn(list, NULL, "Services: Running");

    lv_obj_t *back_btn = lv_btn_create(screen.root);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_btn_label = lv_label_create(back_btn);
    lv_label_set_text(back_btn_label, "Back");

    return screen;
}
