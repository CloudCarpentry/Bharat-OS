/* SPDX-License-Identifier: MIT */
#include "ui_screens.h"

extern int bharat_ui_navigate(bharat_screen_id_t target);

static void diagnostics_btn_event_cb(lv_event_t * e) {
    bharat_ui_navigate(BHARAT_SCREEN_DIAGNOSTICS);
}

bharat_ui_screen_t bharat_home_screen_create(lv_obj_t *parent, const bharat_ui_context_t *context) {
    bharat_ui_screen_t screen;
    screen.id = BHARAT_SCREEN_HOME;

    screen.root = lv_obj_create(parent);
    lv_obj_set_size(screen.root, LV_PCT(100), LV_PCT(100));

    lv_obj_set_flex_flow(screen.root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen.root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *title = lv_label_create(screen.root);
    lv_label_set_text(title, "System Summary");

    lv_obj_t *arch_label = lv_label_create(screen.root);
    lv_label_set_text(arch_label, "Architecture: x86_64");

    lv_obj_t *health_label = lv_label_create(screen.root);
    lv_label_set_text(health_label, "System Health: OK");

    lv_obj_t *diag_btn = lv_btn_create(screen.root);
    lv_obj_add_event_cb(diag_btn, diagnostics_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *diag_btn_label = lv_label_create(diag_btn);
    lv_label_set_text(diag_btn_label, "Diagnostics");

    return screen;
}
