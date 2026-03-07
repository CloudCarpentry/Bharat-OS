#ifndef BHARAT_AI_KERNEL_BRIDGE_H
#define BHARAT_AI_KERNEL_BRIDGE_H

#include <stdint.h>

#include "advanced/ai_sched.h"
#include "capability.h"

int ai_kernel_create_governor_endpoint(capability_table_t* table, uint32_t* out_send_cap, uint32_t* out_recv_cap);
int ai_kernel_ingest_suggestion_ipc(capability_table_t* table, uint32_t recv_cap);

int ai_kernel_apply_suggestion(const ai_suggestion_t* suggestion);
int ai_kernel_collect_telemetry(kernel_telemetry_t* out);

#endif
