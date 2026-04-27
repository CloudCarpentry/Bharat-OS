#ifndef BHARAT_UAPI_TRACE_EVENTS_H
#define BHARAT_UAPI_TRACE_EVENTS_H

#include <stdint.h>
#include <stddef.h>

typedef enum bh_trace_event_id {
    BH_TRACE_AUTO_TASK_START = 0x5000,
    BH_TRACE_AUTO_TASK_END,
    BH_TRACE_AUTO_DEADLINE_MISS,
    BH_TRACE_AUTO_WATCHDOG_HEARTBEAT,
    BH_TRACE_AUTO_WATCHDOG_MISS,
    BH_TRACE_AUTO_CAN_RX,
    BH_TRACE_AUTO_CAN_TX,
    BH_TRACE_AUTO_SAFE_STATE_ENTER,
    BH_TRACE_AUTO_EMU_TICK,
} bh_trace_event_id_t;

typedef struct bh_trace_record {
    uint64_t timestamp_ns;
    uint32_t cpu_id;
    uint32_t event_id;
    uint64_t arg0;
    uint64_t arg1;
    uint64_t arg2;
} bh_trace_record_t;

#endif // BHARAT_UAPI_TRACE_EVENTS_H
