#pragma once

#include "console_discovery.h"

typedef struct {
    int early_primary_index;
    int runtime_primary_index;
    int runtime_secondary_index;
    int panic_primary_index;
    int panic_secondary_index;
} console_policy_decision_t;

void console_choose_policy(const console_device_desc_t *devs,
                           size_t count,
                           console_policy_decision_t *decision);
