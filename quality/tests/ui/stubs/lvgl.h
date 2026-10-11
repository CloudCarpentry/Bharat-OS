/* SPDX-License-Identifier: MIT */
#ifndef TEST_LVGL_STUB_H
#define TEST_LVGL_STUB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _lv_obj_t {
    int dummy;
};
typedef struct _lv_obj_t lv_obj_t;

struct _lv_group_t {
    lv_obj_t *focused;
    int obj_count;
};
typedef struct _lv_group_t lv_group_t;

struct _lv_timer_t {
    void (*timer_cb)(struct _lv_timer_t *);
    uint32_t period;
    bool active;
};
typedef struct _lv_timer_t lv_timer_t;

typedef struct lv_event_t lv_event_t;

typedef uint32_t lv_color_t;
typedef uint32_t lv_style_selector_t;
typedef uint32_t lv_state_t;
typedef uint32_t lv_part_t;

#define LV_PCT(x) (x)
#define LV_OPA_TRANSP 0
#define LV_OBJ_FLAG_SCROLLABLE (1 << 0)
#define LV_ALIGN_TOP_LEFT 0
#define LV_ALIGN_TOP_MID 1
#define LV_ALIGN_CENTER 2
#define LV_ALIGN_BOTTOM_LEFT 3
#define LV_ALIGN_BOTTOM_RIGHT 4
#define LV_FLEX_FLOW_ROW_WRAP 0
#define LV_FLEX_ALIGN_SPACE_EVENLY 0
#define LV_FLEX_ALIGN_CENTER 0
#define LV_PART_MAIN 0
#define LV_PART_INDICATOR 1
#define LV_STATE_FOCUSED 1
#define LV_STATE_PRESSED 2
#define LV_EVENT_CLICKED 1
#define LV_EVENT_KEY 2
#define LV_KEY_ESC 27

extern const int lv_font_montserrat_14;

lv_obj_t *lv_screen_active(void);
lv_obj_t *lv_obj_create(lv_obj_t *parent);
lv_obj_t *lv_btn_create(lv_obj_t *parent);
lv_obj_t *lv_label_create(lv_obj_t *parent);
lv_obj_t *lv_spinner_create(lv_obj_t *parent);

void lv_obj_set_size(lv_obj_t *obj, int32_t w, int32_t h);
void lv_obj_align(lv_obj_t *obj, int align, int32_t x, int32_t y);
void lv_obj_center(lv_obj_t *obj);
void lv_obj_remove_flag(lv_obj_t *obj, int flag);
void lv_obj_delete(lv_obj_t *obj);

void lv_obj_set_style_bg_color(lv_obj_t *obj, lv_color_t color, lv_style_selector_t selector);
void lv_obj_set_style_text_color(lv_obj_t *obj, lv_color_t color, lv_style_selector_t selector);
void lv_obj_set_style_border_width(lv_obj_t *obj, int32_t w, lv_style_selector_t selector);
void lv_obj_set_style_border_color(lv_obj_t *obj, lv_color_t color, lv_style_selector_t selector);
void lv_obj_set_style_radius(lv_obj_t *obj, int32_t r, lv_style_selector_t selector);
void lv_obj_set_style_shadow_width(lv_obj_t *obj, int32_t w, lv_style_selector_t selector);
void lv_obj_set_style_outline_width(lv_obj_t *obj, int32_t w, lv_style_selector_t selector);
void lv_obj_set_style_outline_color(lv_obj_t *obj, lv_color_t color, lv_style_selector_t selector);
void lv_obj_set_style_outline_pad(lv_obj_t *obj, int32_t pad, lv_style_selector_t selector);
void lv_obj_set_style_bg_opa(lv_obj_t *obj, int opa, lv_style_selector_t selector);
void lv_obj_set_style_arc_color(lv_obj_t *obj, lv_color_t color, lv_style_selector_t selector);
void lv_obj_set_style_text_font(lv_obj_t *obj, const void *font, lv_style_selector_t selector);

void lv_obj_set_flex_flow(lv_obj_t *obj, int flow);
void lv_obj_set_flex_align(lv_obj_t *obj, int main, int cross, int track);
void lv_label_set_text(lv_obj_t *obj, const char *text);
void lv_obj_add_event_cb(lv_obj_t *obj, void (*cb)(lv_event_t *), int event, void *user_data);
void *lv_event_get_user_data(lv_event_t *e);
uint32_t lv_event_get_key(lv_event_t *e);

lv_color_t lv_color_hex(uint32_t hex);
int lv_snprintf(char *buf, size_t size, const char *fmt, ...);

lv_group_t *lv_group_create(void);
void lv_group_add_obj(lv_group_t *group, lv_obj_t *obj);
void lv_group_remove_all_objs(lv_group_t *group);
void lv_group_focus_obj(lv_obj_t *obj);
void lv_group_focus_next(lv_group_t *group);

lv_timer_t *lv_timer_create(void (*timer_cb)(lv_timer_t *), uint32_t period, void *user_data);
void lv_timer_delete(lv_timer_t *timer);

#ifdef __cplusplus
}
#endif

#endif /* TEST_LVGL_STUB_H */
