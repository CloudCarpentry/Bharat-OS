#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "stack/can/can_frame.h"

typedef struct {
    uint32_t id;
    uint32_t mask;
    bool match_extended;
    bool extended_only;
} can_filter_t;

bool can_filter_match(const can_filter_t* filter, const can_frame_t* frame);
