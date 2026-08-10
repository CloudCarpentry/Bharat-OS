#include "fbui_input_bridge.h"

#ifndef REL_X
#define REL_X 0x00
#endif
#ifndef REL_Y
#define REL_Y 0x01
#endif
#ifndef BTN_LEFT
#define BTN_LEFT 0x110
#endif

#define EV_SYN 0x00
#define EV_KEY 0x01
#define EV_REL 0x02

void fbui_input_bridge_init(fbui_input_bridge_t *bridge, int32_t width, int32_t height) {
    if (!bridge) return;
    bridge->cursor_x = width / 2;
    bridge->cursor_y = height / 2;
    bridge->width = width;
    bridge->height = height;
    bridge->last_left_button = false;
    bridge->accumulated_dx = 0;
    bridge->accumulated_dy = 0;
}

bool fbui_input_bridge_translate(fbui_input_bridge_t *bridge,
                                 const bh_input_event_t *iev,
                                 fbui_event_t *oev) {
    if (!bridge || !iev || !oev) return false;

    oev->x = bridge->cursor_x;
    oev->y = bridge->cursor_y;
    oev->keycode = 0;

    switch (iev->type) {
        case EV_REL:
            if (iev->code == REL_X) {
                bridge->accumulated_dx += iev->value;
            } else if (iev->code == REL_Y) {
                bridge->accumulated_dy += iev->value;
            }
            return false; // Accumulate until EV_SYN

        case EV_SYN:
            // Flush relative mouse changes on SYN_REPORT
            if (bridge->accumulated_dx != 0 || bridge->accumulated_dy != 0) {
                bridge->cursor_x += bridge->accumulated_dx;
                bridge->cursor_y += bridge->accumulated_dy;

                // Clamp cursor to framebuffer bounds
                if (bridge->cursor_x < 0) bridge->cursor_x = 0;
                if (bridge->cursor_x >= bridge->width) bridge->cursor_x = bridge->width - 1;
                if (bridge->cursor_y < 0) bridge->cursor_y = 0;
                if (bridge->cursor_y >= bridge->height) bridge->cursor_y = bridge->height - 1;

                bridge->accumulated_dx = 0;
                bridge->accumulated_dy = 0;

                oev->type = FBUI_EVENT_POINTER_MOVE;
                oev->x = bridge->cursor_x;
                oev->y = bridge->cursor_y;
                return true;
            }
            return false;

        case EV_KEY:
            if (iev->code == BTN_LEFT) {
                bridge->last_left_button = (iev->value != 0);
                oev->type = (iev->value != 0) ? FBUI_EVENT_POINTER_DOWN : FBUI_EVENT_POINTER_UP;
                oev->x = bridge->cursor_x;
                oev->y = bridge->cursor_y;
                return true;
            } else {
                oev->type = (iev->value != 0) ? FBUI_EVENT_KEY_DOWN : FBUI_EVENT_KEY_UP;
                oev->keycode = iev->code;
                return true;
            }

        default:
            return false;
    }
}
