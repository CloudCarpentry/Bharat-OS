#ifndef BHARAT_FAST_TRACE_H
#define BHARAT_FAST_TRACE_H

#include <bharat/uapi/trace/bh_trace_events.h>
#include <stddef.h>

void bh_fast_trace_init(void);
void bh_fast_trace_emit(uint32_t event_id, uint64_t arg0, uint64_t arg1, uint64_t arg2);
size_t bh_fast_trace_snapshot(bh_trace_record_t *out, size_t max_records);
void bh_fast_trace_reset(void);
uint32_t bh_fast_trace_get_overflow_count(void);

#endif // BHARAT_FAST_TRACE_H
