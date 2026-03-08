#include "bharat/ui/fbui_events.h"
#include <stddef.h>

void fbui_event_loop_init(fbui_event_loop_t *loop, fbui_widget_t *root) {
    if (!loop) return;
    loop->root_widget = root;
    loop->focused_widget = NULL;
}

/**
 * Dispatch an input event down the widget tree.
 * Simple depth-first search of sibling lists.
 */
static bool dispatch_recursive(fbui_widget_t *node, const fbui_event_t *ev) {
    if (!node || !node->visible) return false;

    // Dispatch to children first (if it had any, typically handled in a panel widget)
    // For this simple skeleton, we assume linear sibling list.
    if (node->next) {
        if (dispatch_recursive(node->next, ev)) {
            return true;
        }
    }

    // Process event locally
    if (node->ops && node->ops->handle_event) {
        if (node->ops->handle_event(node, ev)) {
            return true; // Event consumed
        }
    }

    return false;
}

void fbui_dispatch_event(fbui_event_loop_t *loop, const fbui_event_t *ev) {
    if (!loop || !loop->root_widget || !ev) return;

    // In a real GUI, mouse/touch events hit-test top-down (Z-order),
    // while keyboard events go directly to the focused widget.

    if (ev->type == FBUI_EVENT_KEY_PRESS && loop->focused_widget) {
        if (loop->focused_widget->ops && loop->focused_widget->ops->handle_event) {
            loop->focused_widget->ops->handle_event(loop->focused_widget, ev);
        }
    } else {
        // Touch or un-focused dispatch
        dispatch_recursive(loop->root_widget, ev);
    }
}
