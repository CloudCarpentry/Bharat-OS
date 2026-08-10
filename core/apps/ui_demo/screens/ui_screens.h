/* SPDX-License-Identifier: MIT */
#ifndef BHARAT_UI_SCREENS_H
#define BHARAT_UI_SCREENS_H

#include "lvgl.h"

typedef enum {
    BHARAT_SCREEN_SPLASH = 0,
    BHARAT_SCREEN_HOME,
    BHARAT_SCREEN_DIAGNOSTICS,
    BHARAT_SCREEN_RECOVERY,
    BHARAT_SCREEN_SETTINGS,
    BHARAT_SCREEN_COUNT
} bharat_screen_id_t;

typedef struct {
    /* Optional mock context for the demo data */
} bharat_ui_context_t;

typedef struct {
    lv_obj_t *root;
    bharat_screen_id_t id;
} bharat_ui_screen_t;

bharat_ui_screen_t bharat_splash_screen_create(lv_obj_t *parent, const bharat_ui_context_t *context);
bharat_ui_screen_t bharat_home_screen_create(lv_obj_t *parent, const bharat_ui_context_t *context);
bharat_ui_screen_t bharat_diagnostics_screen_create(lv_obj_t *parent, const bharat_ui_context_t *context);
bharat_ui_screen_t bharat_recovery_screen_create(lv_obj_t *parent, const bharat_ui_context_t *context);
bharat_ui_screen_t bharat_settings_screen_create(lv_obj_t *parent, const bharat_ui_context_t *context);

/* Splash update functions */
void bharat_splash_set_progress(bharat_ui_screen_t *screen, uint8_t percentage);
void bharat_splash_set_stage(bharat_ui_screen_t *screen, const char *stage);

#endif /* BHARAT_UI_SCREENS_H */
