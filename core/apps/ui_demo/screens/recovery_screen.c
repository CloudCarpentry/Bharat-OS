/* SPDX-License-Identifier: MIT */
#include "ui_screens.h"

static void msgbox_event_cb(lv_event_t * e) {
    lv_obj_t * msgbox = lv_event_get_current_target(e);
    if(lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        lv_msgbox_close(msgbox);
    }
}

static void btn_event_cb(lv_event_t * e) {
    lv_obj_t * parent = lv_obj_get_screen(lv_event_get_target(e));
    lv_obj_t * mbox = lv_msgbox_create(parent);
    lv_msgbox_add_title(mbox, "Confirm Action");
    lv_msgbox_add_text(mbox, "Are you sure you want to perform this destructive action?");
    lv_obj_t * btn = lv_msgbox_add_footer_button(mbox, "Yes");
    lv_obj_t * btn2 = lv_msgbox_add_footer_button(mbox, "Cancel");
    lv_obj_add_event_cb(mbox, msgbox_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

bharat_ui_screen_t bharat_recovery_screen_create(lv_obj_t *parent, const bharat_ui_context_t *context) {
    bharat_ui_screen_t screen;
    screen.id = BHARAT_SCREEN_RECOVERY;

    screen.root = lv_obj_create(parent);
    lv_obj_set_size(screen.root, LV_PCT(100), LV_PCT(100));

    lv_obj_set_flex_flow(screen.root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen.root, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *title = lv_label_create(screen.root);
    lv_label_set_text(title, "Recovery Mode");

    lv_obj_t *btn_retry = lv_btn_create(screen.root);
    lv_obj_t *lbl_retry = lv_label_create(btn_retry);
    lv_label_set_text(lbl_retry, "Retry Boot");
    lv_obj_add_event_cb(btn_retry, btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_reboot = lv_btn_create(screen.root);
    lv_obj_t *lbl_reboot = lv_label_create(btn_reboot);
    lv_label_set_text(lbl_reboot, "Reboot");
    lv_obj_add_event_cb(btn_reboot, btn_event_cb, LV_EVENT_CLICKED, NULL);

    return screen;
}
