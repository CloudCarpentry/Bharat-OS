#ifndef BHARAT_UAPI_SYSTEM_TELEMETRY_H
#define BHARAT_UAPI_SYSTEM_TELEMETRY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file telemetry.h
 * @brief UAPI for System Telemetry and Diagnostics.
 *
 * This file defines the stable, shared data structures for telemetry events,
 * counters, and diagnostic data across the system. It adheres to the strict
 * UAPI boundary definition where these structs describe *what* the data is.
 */

/* --------------------------------------------------------------------------
 * Event Kinds
 * -------------------------------------------------------------------------- */
#define BHARAT_EVENT_KIND_FAULT      1
#define BHARAT_EVENT_KIND_POWER      2
#define BHARAT_EVENT_KIND_THERMAL    3
#define BHARAT_EVENT_KIND_NETWORK    4
#define BHARAT_EVENT_KIND_HEARTBEAT  5
#define BHARAT_EVENT_KIND_AUDIT      6
#define BHARAT_EVENT_KIND_DIAG       7

/* --------------------------------------------------------------------------
 * Event Severities
 * -------------------------------------------------------------------------- */
#define BHARAT_SEVERITY_DEBUG        0
#define BHARAT_SEVERITY_INFO         1
#define BHARAT_SEVERITY_WARNING      2
#define BHARAT_SEVERITY_ERROR        3
#define BHARAT_SEVERITY_CRITICAL     4
#define BHARAT_SEVERITY_FATAL        5

/* --------------------------------------------------------------------------
 * Telemetry Event Structure
 * -------------------------------------------------------------------------- */
typedef struct {
    uint32_t version;         /* Event format version (currently 1) */
    uint32_t kind;            /* Event kind (e.g., BHARAT_EVENT_KIND_FAULT) */
    uint64_t timestamp_ns;    /* Monotonic timestamp in nanoseconds */
    uint32_t severity;        /* Event severity */
    uint32_t source_id;       /* Domain or subsystem ID originating the event */
    uint32_t payload_type;    /* Identifies the layout of payload_data */
    uint32_t payload_len;     /* Length of the payload */
    uint8_t  payload_data[64]; /* Inline payload data (union or typed extension could go here) */
} bharat_telemetry_event_t;


/* --------------------------------------------------------------------------
 * Heartbeat Specifics
 * -------------------------------------------------------------------------- */
/* When kind == BHARAT_EVENT_KIND_HEARTBEAT, the payload represents this struct */
typedef struct {
    uint32_t status_flags;    /* Bitmask indicating component health */
    uint32_t uptime_seconds;
} bharat_heartbeat_payload_t;


/* --------------------------------------------------------------------------
 * Counter Definitions
 * -------------------------------------------------------------------------- */
#define BHARAT_COUNTER_TYPE_MONOTONIC 0
#define BHARAT_COUNTER_TYPE_GAUGE     1

typedef struct {
    uint32_t counter_id;
    uint32_t source_id;       /* Subsystem or component ID */
    uint32_t type;            /* BHARAT_COUNTER_TYPE_MONOTONIC or GAUGE */
    char     name[32];        /* Null-terminated name */
    char     unit[16];        /* e.g., "bytes", "errors", "celsius" */
} bharat_counter_desc_t;

typedef struct {
    uint32_t counter_id;
    uint64_t value;           /* Current counter value */
    uint64_t timestamp_ns;    /* When this value was sampled */
} bharat_counter_snapshot_t;


/* --------------------------------------------------------------------------
 * Subscription Filters
 * -------------------------------------------------------------------------- */
typedef struct {
    uint32_t event_kind_mask; /* Bitmask of event kinds to receive, or 0xFFFFFFFF for all */
    uint32_t min_severity;    /* Only receive events with severity >= min_severity */
    uint32_t source_id;       /* Specific source ID to filter, or 0 for all sources */
} bharat_telemetry_filter_t;


#ifdef __cplusplus
}
#endif

#endif /* BHARAT_UAPI_SYSTEM_TELEMETRY_H */