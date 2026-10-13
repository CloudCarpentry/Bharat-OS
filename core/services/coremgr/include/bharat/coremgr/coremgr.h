#ifndef BHARAT_COREMGR_H
#define BHARAT_COREMGR_H

#include <stdint.h>
#include <stdbool.h>

#define COREMGR_MAX_CORES 32U

typedef enum {
    CORE_STATE_UNKNOWN = 0,
    CORE_STATE_DISCOVERED,
    CORE_STATE_STARTING,
    CORE_STATE_ONLINE,
    CORE_STATE_DEGRADED,
    CORE_STATE_OFFLINE,
    CORE_STATE_FAILED
} coremgr_core_state_t;

typedef enum {
    CORE_TRANSPORT_HEALTHY = 0,
    CORE_TRANSPORT_DEGRADED,
    CORE_TRANSPORT_FAILED
} coremgr_transport_state_t;

typedef struct {
    uint32_t core_id;
    uint32_t arch_id;
    bool is_online;
    coremgr_core_state_t lifecycle_state;
    uint64_t last_health_ts;
    coremgr_transport_state_t transport_state;
    uint32_t last_failure_reason;
    uint32_t topology_version;
} coremgr_core_info_t;

typedef struct {
    uint32_t total_detected;
    uint32_t online_count;
    uint32_t degraded_count;
    uint32_t failed_count;
    coremgr_transport_state_t transport_status;
    uint32_t registry_overflows;
    uint32_t invalid_events;
} coremgr_diagnostics_t;

/* Registry API */
void coremgr_registry_init(void);
int coremgr_register_core(uint32_t core_id, uint32_t arch_id);
int coremgr_get_core_info(uint32_t core_id, coremgr_core_info_t *out_info);

/* Lifecycle API */
int coremgr_set_core_state(uint32_t core_id, coremgr_core_state_t new_state);
int coremgr_set_transport_state(uint32_t core_id, coremgr_transport_state_t new_state);
int coremgr_record_failure(uint32_t core_id, uint32_t reason);

/* Diagnostics API */
coremgr_diagnostics_t coremgr_get_diagnostics(void);
void coremgr_inc_invalid_event(void);
void coremgr_inc_registry_overflow(void);

/* Event processing */
int coremgr_process_event(void);

/* Internal API for lifecycle.c */
coremgr_core_info_t* coremgr_get_core_info_ptr(uint32_t core_id);
coremgr_core_info_t* coremgr_get_core_info_by_index(uint32_t index);

#endif // BHARAT_COREMGR_H
