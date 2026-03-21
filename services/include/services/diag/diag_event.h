#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    DIAG_SEV_INFO = 0,
    DIAG_SEV_WARN,
    DIAG_SEV_DEGRADED,
    DIAG_SEV_CRITICAL
} diag_severity_t;

typedef struct {
    uint32_t dtc;
    diag_severity_t severity;
    uint64_t timestamp_ns;
    uint32_t source_id;
    uint32_t snapshot_id;
} diag_event_t;

int diag_report_event(const diag_event_t* event);
int diag_clear_event(uint32_t dtc);
int diag_get_event(uint32_t dtc, diag_event_t* out_event);
