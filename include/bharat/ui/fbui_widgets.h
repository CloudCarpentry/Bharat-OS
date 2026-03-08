#ifndef BHARAT_FBUI_WIDGETS_H
#define BHARAT_FBUI_WIDGETS_H

#include "bharat/ui/fb_render.h"
#include <stdbool.h>

/**
 * Basic UI Event
 */
typedef enum {
    FBUI_EVENT_TOUCH_DOWN,
    FBUI_EVENT_TOUCH_UP,
    FBUI_EVENT_TOUCH_MOVE,
    FBUI_EVENT_KEY_PRESS
} fbui_event_type_t;

typedef struct {
    fbui_event_type_t type;
    int x;
    int y;
    int keycode;
} fbui_event_t;

/**
 * Base Widget Types
 */
typedef enum {
    FBUI_WIDGET_LABEL,
    FBUI_WIDGET_BUTTON,
    FBUI_WIDGET_PROGRESS,
    FBUI_WIDGET_PANEL
} fbui_widget_type_t;

struct fbui_widget;

/**
 * Widget Virtual Method Table
 */
typedef struct {
    void (*draw)(struct fbui_widget *widget, fbui_render_context_t *ctx);
    bool (*handle_event)(struct fbui_widget *widget, const fbui_event_t *ev);
} fbui_widget_ops_t;

/**
 * Base Widget Structure
 */
typedef struct fbui_widget {
    fbui_widget_type_t type;
    int x, y, width, height;
    bool visible;
    bool focused;

    // Theme colors
    uint32_t bg_color;
    uint32_t fg_color;
    uint32_t border_color;

    const char *text;
    const fbui_widget_ops_t *ops;
    void *priv_data; // Child-specific data

    // Tree (Simple singly-linked list for sibling chaining)
    struct fbui_widget *next;
} fbui_widget_t;

/**
 * Basic Factory and Layout Methods
 */
void fbui_widget_init(fbui_widget_t *w, fbui_widget_type_t type, int x, int y, int width, int height);
void fbui_widget_set_text(fbui_widget_t *w, const char *text);
bool fbui_widget_hit_test(const fbui_widget_t *w, int px, int py);

// Specific Widget Factories
fbui_widget_t* fbui_create_button(int x, int y, int w, int h, const char *text);
fbui_widget_t* fbui_create_label(int x, int y, int w, int h, const char *text);
fbui_widget_t* fbui_create_progress(int x, int y, int w, int h, float value);

#endif // BHARAT_FBUI_WIDGETS_H
