#ifndef BHARAT_TRACE_H
#define BHARAT_TRACE_H

#include <stdint.h>

typedef enum {
    TRACE_EVENT_CTX_SWITCH,
    TRACE_EVENT_IRQ_ENTRY,
    TRACE_EVENT_IRQ_EXIT,
    TRACE_EVENT_SYSCALL,
} trace_event_type_t;

typedef struct {
    uint64_t timestamp;
    trace_event_type_t type;
    uint64_t arg1;
    uint64_t arg2;
    uint64_t arg3;
} trace_event_t;

// Initialize tracing subsystem
void trace_init(void);

// Emit a trace event
void trace_emit(trace_event_type_t type, uint64_t a1, uint64_t a2, uint64_t a3);

// Dump trace buffer via hal_uart_puts
void trace_dump(void);

#endif
