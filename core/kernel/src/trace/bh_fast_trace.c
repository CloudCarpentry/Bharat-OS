#include <trace/bh_fast_trace.h>
#include <lib/base/string.h>

#define BH_FAST_TRACE_BUFFER_SIZE 2048

static bh_trace_record_t g_trace_buffer[BH_FAST_TRACE_BUFFER_SIZE];
static uint32_t g_trace_head = 0;
static uint32_t g_trace_count = 0;
static uint32_t g_overflow_count = 0;

void bh_fast_trace_init(void) {
    g_trace_head = 0;
    g_trace_count = 0;
    g_overflow_count = 0;
    for (size_t i = 0; i < BH_FAST_TRACE_BUFFER_SIZE; i++) {
        g_trace_buffer[i].event_id = 0;
    }
}

void bh_fast_trace_emit(uint32_t event_id, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    uint32_t idx = g_trace_head;

    g_trace_buffer[idx].timestamp_ns = 0;
    g_trace_buffer[idx].cpu_id = 0;
    g_trace_buffer[idx].event_id = event_id;
    g_trace_buffer[idx].arg0 = arg0;
    g_trace_buffer[idx].arg1 = arg1;
    g_trace_buffer[idx].arg2 = arg2;

    g_trace_head = (g_trace_head + 1) % BH_FAST_TRACE_BUFFER_SIZE;
    if (g_trace_count < BH_FAST_TRACE_BUFFER_SIZE) {
        g_trace_count++;
    } else {
        g_overflow_count++;
    }
}

size_t bh_fast_trace_snapshot(bh_trace_record_t *out, size_t max_records) {
    if (!out || max_records == 0) return 0;
    size_t to_copy = (g_trace_count < (uint32_t)max_records) ? g_trace_count : (uint32_t)max_records;
    uint32_t start_idx;
    if (g_trace_count < BH_FAST_TRACE_BUFFER_SIZE) {
        start_idx = 0;
    } else {
        start_idx = g_trace_head;
    }
    for (size_t i = 0; i < to_copy; i++) {
        uint32_t idx = (start_idx + i) % BH_FAST_TRACE_BUFFER_SIZE;
        out[i] = g_trace_buffer[idx];
    }
    return to_copy;
}

void bh_fast_trace_reset(void) {
    g_trace_head = 0;
    g_trace_count = 0;
    g_overflow_count = 0;
}

uint32_t bh_fast_trace_get_overflow_count(void) {
    return g_overflow_count;
}
