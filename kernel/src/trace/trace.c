#include "trace/trace.h"
#include "hal/timer.h"
#include "hal/serial.h"

#define TRACE_BUFFER_SIZE 1024

static trace_event_t trace_buffer[TRACE_BUFFER_SIZE];
static uint32_t trace_head = 0;
static uint32_t trace_tail = 0;

void trace_init(void) {
    trace_head = 0;
    trace_tail = 0;
}

void trace_emit(trace_event_type_t type, uint64_t a1, uint64_t a2, uint64_t a3) {
    uint32_t head = trace_head;
    uint32_t next = (head + 1) % TRACE_BUFFER_SIZE;

    // Placeholder for timer reading as baremetal HAL doesn't expose get_ms yet
    trace_buffer[head].timestamp = 0;
    trace_buffer[head].type = type;
    trace_buffer[head].arg1 = a1;
    trace_buffer[head].arg2 = a2;
    trace_buffer[head].arg3 = a3;

    trace_head = next;
    if (next == trace_tail) {
        // Buffer full, drop oldest
        trace_tail = (trace_tail + 1) % TRACE_BUFFER_SIZE;
    }
}

void trace_dump(void) {
    uint32_t curr = trace_tail;
    while(curr != trace_head) {
        trace_event_t *ev = &trace_buffer[curr];
        (void)ev; // normally format to hal_serial_puts
        curr = (curr + 1) % TRACE_BUFFER_SIZE;
    }
}
