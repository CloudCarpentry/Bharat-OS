#include <stddef.h>

#include "stack/can/can_filter.h"

bool can_filter_match(const can_filter_t* filter, const can_frame_t* frame);

int can_filter_validate(const can_filter_t* filter) {
    if (!filter) {
        return -1;
    }
    if (!filter->match_extended && filter->extended_only) {
        return -1;
    }
    return 0;
}

int can_filter_compile_list(const can_filter_t* filters, size_t count) {
    size_t i;

    if (!filters && count != 0) {
        return -1;
    }
    for (i = 0; i < count; ++i) {
        if (can_filter_validate(&filters[i]) != 0) {
            return -1;
        }
    }
    return 0;
}
