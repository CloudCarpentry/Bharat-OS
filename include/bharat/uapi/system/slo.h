#ifndef BHARAT_UAPI_SYSTEM_SLO_H
#define BHARAT_UAPI_SYSTEM_SLO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Bharat-OS Service Level Objective (SLO) Definitions
 * Establishes measurable gates for unified policy enforcement.
 */

typedef struct {
    uint32_t max_dispatch_latency_us;  /* Max allowable scheduler dispatch latency */
    uint32_t max_runqueue_depth;       /* Max threads in runqueue before load-shedding/migration */
    uint32_t max_ipc_queue_depth;      /* Max pending IPC messages before backpressure */
    uint32_t mem_pressure_threshold_pct; /* Percentage of memory used to trigger reclaim/OOM */
    uint32_t watchdog_miss_threshold;  /* Allowable missed heartbeats before fault action */
} bharat_slo_gates_t;

typedef enum {
    BHARAT_SLO_STATE_OK = 0,
    BHARAT_SLO_STATE_WARNING = 1,
    BHARAT_SLO_STATE_VIOLATED = 2
} bharat_slo_state_t;

/* Used for reporting SLO status to policymgr / telemetrymgr */
typedef struct {
    uint32_t subsystem_id;
    bharat_slo_state_t current_state;
    uint32_t violation_count;
} bharat_slo_report_t;

#ifdef __cplusplus
}
#endif

#endif // BHARAT_UAPI_SYSTEM_SLO_H
