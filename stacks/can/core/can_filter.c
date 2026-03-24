#include "stack/can/can_filter.h"

bool can_filter_match(const can_filter_t* filter, const can_frame_t* frame) {
    if (!filter || !frame) {
        return false;
    }

    if (filter->extended_only && !frame->is_extended) {
        return false;
    }

    if (!filter->match_extended && frame->is_extended) {
        return false;
    }

    return (frame->id & filter->mask) == (filter->id & filter->mask);
}
