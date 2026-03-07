#ifndef BHARAT_AI_KERNEL_BRIDGE_H
#define BHARAT_AI_KERNEL_BRIDGE_H

#include <stdint.h>

#include "advanced/ai_sched.h"

int ai_kernel_apply_suggestion(const ai_suggestion_t* suggestion);
int ai_kernel_collect_telemetry(kernel_telemetry_t* out);

#endif
