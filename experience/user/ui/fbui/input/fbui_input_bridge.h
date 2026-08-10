#ifndef FBUI_INPUT_BRIDGE_H
#define FBUI_INPUT_BRIDGE_H

#include "bharat/uapi/input/input_event.h"
#include "bharat/ui/fbui_widgets.h"

typedef struct {
    int32_t cursor_x;
    int32_t cursor_y;
    int32_t width;
    int32_t height;

    bool last_left_button;
    int32_t accumulated_dx;
    int32_t accumulated_dy;
} fbui_input_bridge_t;

void fbui_input_bridge_init(fbui_input_bridge_t *bridge, int32_t width, int32_t height);

/**
 * Translates a normalized bh_input_event_t into an fbui_event_t.
 * Returns true if a valid FBUI event is translated, false if ignored/accumulating.
 */
bool fbui_input_bridge_translate(fbui_input_bridge_t *bridge,
                                 const bh_input_event_t *iev,
                                 fbui_event_t *oev);

#endif // FBUI_INPUT_BRIDGE_H
