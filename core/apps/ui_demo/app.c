/* SPDX-License-Identifier: MIT */
#include "lvgl.h"
#include "ui_screens.h"
#include <stdio.h>

#define HISTORY_SIZE 10

static bharat_screen_id_t screen_history[HISTORY_SIZE];
static int history_index = -1;
static bharat_ui_screen_t current_screen;
static lv_obj_t * main_parent = NULL;

static void load_screen(bharat_screen_id_t target) {
    if (current_screen.root != NULL) {
        lv_obj_del(current_screen.root);
        current_screen.root = NULL;
    }

    switch(target) {
        case BHARAT_SCREEN_SPLASH:
            current_screen = bharat_splash_screen_create(main_parent, NULL);
            break;
        case BHARAT_SCREEN_HOME:
            current_screen = bharat_home_screen_create(main_parent, NULL);
            break;
        case BHARAT_SCREEN_DIAGNOSTICS:
            current_screen = bharat_diagnostics_screen_create(main_parent, NULL);
            break;
        case BHARAT_SCREEN_RECOVERY:
            current_screen = bharat_recovery_screen_create(main_parent, NULL);
            break;
        case BHARAT_SCREEN_SETTINGS:
            current_screen = bharat_settings_screen_create(main_parent, NULL);
            break;
        default:
            return;
    }
}

int bharat_ui_navigate(bharat_screen_id_t target) {
    if (history_index < HISTORY_SIZE - 1) {
        if(history_index >= 0) {
            screen_history[history_index] = current_screen.id;
        }
        history_index++;
        load_screen(target);
        return 0;
    }
    return -1;
}

int bharat_ui_navigate_back(void) {
    if (history_index > 0) {
        history_index--;
        bharat_screen_id_t target = screen_history[history_index];
        load_screen(target);
        return 0;
    }
    return -1;
}

bharat_screen_id_t bharat_ui_current_screen(void) {
    return current_screen.id;
}

void bharat_ui_app_start(void) {
    main_parent = lv_screen_active();
    history_index = -1;
    current_screen.root = NULL;
    bharat_ui_navigate(BHARAT_SCREEN_SPLASH);
}
