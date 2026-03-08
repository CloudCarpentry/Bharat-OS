#include "bharat/ui/fbui_widgets.h"
#include "bharat/ui/fb_render.h"
#include <stddef.h>
#include <string.h>

// Helper to determine if a point is within widget bounds
bool fbui_widget_hit_test(const fbui_widget_t *w, int px, int py) {
    if (!w) return false;
    return (px >= w->x && px <= w->x + w->width && py >= w->y && py <= w->y + w->height);
}

// Common initializer
void fbui_widget_init(fbui_widget_t *w, fbui_widget_type_t type, int x, int y, int width, int height) {
    w->type = type;
    w->x = x;
    w->y = y;
    w->width = width;
    w->height = height;
    w->visible = true;
    w->focused = false;
    w->next = NULL;
    w->priv_data = NULL;
    w->text = NULL;

    w->bg_color = 0xFFCCCCCC; // Light gray default
    w->fg_color = 0xFF000000; // Black text default
    w->border_color = 0xFF888888; // Dark gray border
}

void fbui_widget_set_text(fbui_widget_t *w, const char *text) {
    if (w) w->text = text;
}

/**
 * LABEL WIDGET
 */
static void label_draw(fbui_widget_t *w, fbui_render_context_t *ctx) {
    if (!w->visible) return;
    // Draw background if not fully transparent (simplified)
    fbui_render_fill_rect(ctx, w->x, w->y, w->width, w->height, w->bg_color);

    // In a real system, invoke a font rasterizer here to draw `w->text`.
    // We just indicate a simple text string outline.
    if (w->text) {
        // Placeholder text render indicator (small rect in the corner)
        fbui_render_fill_rect(ctx, w->x + 2, w->y + 2, 8, 16, w->fg_color);
    }
}

static bool label_handle_event(fbui_widget_t *w, const fbui_event_t *ev) {
    return false; // Labels usually don't consume events
}

static const fbui_widget_ops_t label_ops = {
    .draw = label_draw,
    .handle_event = label_handle_event
};

// Assuming static allocation for testing skeleton (kmalloc would be used here)
static fbui_widget_t _dummy_label_pool[10];
static int _label_cnt = 0;

fbui_widget_t* fbui_create_label(int x, int y, int w, int h, const char *text) {
    if (_label_cnt >= 10) return NULL;
    fbui_widget_t *lbl = &_dummy_label_pool[_label_cnt++];
    fbui_widget_init(lbl, FBUI_WIDGET_LABEL, x, y, w, h);
    lbl->text = text;
    lbl->bg_color = 0xFFFFFFFF; // White
    lbl->ops = &label_ops;
    return lbl;
}

/**
 * BUTTON WIDGET
 */
static void button_draw(fbui_widget_t *w, fbui_render_context_t *ctx) {
    if (!w->visible) return;

    // Change color on press state
    uint32_t bg = w->focused ? 0xFFAAAAAA : w->bg_color;

    fbui_render_fill_rect(ctx, w->x, w->y, w->width, w->height, w->border_color);
    fbui_render_fill_rect(ctx, w->x + 2, w->y + 2, w->width - 4, w->height - 4, bg);

    if (w->text) {
        // Placeholder text
        fbui_render_fill_rect(ctx, w->x + w->width/2 - 4, w->y + w->height/2 - 8, 8, 16, w->fg_color);
    }
}

static bool button_handle_event(fbui_widget_t *w, const fbui_event_t *ev) {
    if (ev->type == FBUI_EVENT_TOUCH_DOWN && fbui_widget_hit_test(w, ev->x, ev->y)) {
        w->focused = true;
        return true; // Event consumed
    } else if (ev->type == FBUI_EVENT_TOUCH_UP) {
        if (w->focused && fbui_widget_hit_test(w, ev->x, ev->y)) {
            // "Clicked" event would be fired to a callback here
        }
        w->focused = false;
        return true; // Consume if it was our button being released
    }
    return false;
}

static const fbui_widget_ops_t button_ops = {
    .draw = button_draw,
    .handle_event = button_handle_event
};

static fbui_widget_t _dummy_btn_pool[10];
static int _btn_cnt = 0;

fbui_widget_t* fbui_create_button(int x, int y, int w, int h, const char *text) {
    if (_btn_cnt >= 10) return NULL;
    fbui_widget_t *btn = &_dummy_btn_pool[_btn_cnt++];
    fbui_widget_init(btn, FBUI_WIDGET_BUTTON, x, y, w, h);
    btn->text = text;
    btn->bg_color = 0xFFDDDDDD; // Light gray
    btn->ops = &button_ops;
    return btn;
}

/**
 * PROGRESS BAR WIDGET
 */
typedef struct {
    float progress; // 0.0 to 1.0
} fbui_progress_data_t;

static void progress_draw(fbui_widget_t *w, fbui_render_context_t *ctx) {
    if (!w->visible) return;

    fbui_progress_data_t *pdata = (fbui_progress_data_t *)w->priv_data;
    if (!pdata) return;

    // Draw border
    fbui_render_fill_rect(ctx, w->x, w->y, w->width, w->height, w->border_color);
    // Draw background
    fbui_render_fill_rect(ctx, w->x + 2, w->y + 2, w->width - 4, w->height - 4, w->bg_color);

    // Draw fill
    int fill_width = (int)((w->width - 4) * pdata->progress);
    if (fill_width > w->width - 4) fill_width = w->width - 4;

    if (fill_width > 0) {
        fbui_render_fill_rect(ctx, w->x + 2, w->y + 2, fill_width, w->height - 4, 0xFF00FF00); // Green fill
    }
}

static const fbui_widget_ops_t progress_ops = {
    .draw = progress_draw,
    .handle_event = NULL
};

static fbui_widget_t _dummy_prog_pool[5];
static fbui_progress_data_t _dummy_prog_data_pool[5];
static int _prog_cnt = 0;

fbui_widget_t* fbui_create_progress(int x, int y, int w, int h, float value) {
    if (_prog_cnt >= 5) return NULL;
    fbui_widget_t *prog = &_dummy_prog_pool[_prog_cnt];
    fbui_progress_data_t *data = &_dummy_prog_data_pool[_prog_cnt++];

    fbui_widget_init(prog, FBUI_WIDGET_PROGRESS, x, y, w, h);
    data->progress = value;
    prog->priv_data = data;
    prog->ops = &progress_ops;
    prog->bg_color = 0xFFEEEEEE;
    return prog;
}
