#ifndef BHARAT_FBUI_EVENTS_H
#define BHARAT_FBUI_EVENTS_H

#include "bharat/ui/fbui_widgets.h"

typedef struct {
    fbui_widget_t *root_widget;
    fbui_widget_t *focused_widget;
} fbui_event_loop_t;

void fbui_event_loop_init(fbui_event_loop_t *loop, fbui_widget_t *root);
void fbui_dispatch_event(fbui_event_loop_t *loop, const fbui_event_t *ev);

#endif // BHARAT_FBUI_EVENTS_H
