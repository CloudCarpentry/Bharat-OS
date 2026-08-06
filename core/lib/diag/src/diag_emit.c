/* SPDX-License-Identifier: MIT */
#include "bharat/diag/diag_emit.h"
#ifndef BHARAT_DIAG_ENABLED
#define BHARAT_DIAG_ENABLED 1
#endif
bh_status_t bh_diag_emit(bh_diag_sink_t *sink, uint16_t event_type, uint8_t severity, uint16_t subsystem_id, const void *payload, uint32_t payload_size) {
#if !BHARAT_DIAG_ENABLED
    (void)sink;(void)event_type;(void)severity;(void)subsystem_id;(void)payload;(void)payload_size; return BH_ERR_NOT_SUPPORTED;
#else
    if (!sink || !sink->ring || !sink->timestamp || sink->source_kind >= BH_DIAG_SOURCE_KIND_COUNT || severity >= BH_DIAG_SEVERITY_COUNT || payload_size > BHARAT_DIAG_MAX_PAYLOAD || (payload_size && !payload)) return BH_ERR_INVALID_ARGUMENT;
    bh_diag_event_header_t h = { .abi_version=BH_DIAG_ABI_VERSION, .header_size=sizeof(h), .event_type=event_type, .severity=severity, .source_kind=sink->source_kind, .payload_size=payload_size, .flags=0, .sequence=0, .timestamp_ns=sink->timestamp(sink->timestamp_context), .source_id=sink->source_id, .cpu_id=sink->cpu_id, .subsystem_id=subsystem_id };
    return bh_diag_ring_try_write(sink->ring, &h, payload);
#endif
}
