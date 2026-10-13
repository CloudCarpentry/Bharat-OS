#ifndef FAULTMGR_STATE_H
#define FAULTMGR_STATE_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/uapi/faultmgr/contract.h>

#define FAULTMGR_MAX_RECORDS 128
#define FAULTMGR_OBSERVATION_WINDOW_NS (5ULL * 1000000000ULL) // 5 seconds

typedef struct {
    uint32_t domain_id;
    uint32_t source_id;
    faultmgr_category_t last_category;
    faultmgr_severity_t highest_severity;
    uint64_t first_occurrence;
    uint64_t latest_occurrence;
    uint32_t failure_count;
    faultmgr_recovery_action_t last_action;
    uint64_t incarnation_id;
    bool active;
} fault_record_t;

void faultmgr_state_init(void);

// Processes a fault event and returns the recommended recovery action.
faultmgr_recovery_action_t faultmgr_process_event(const faultmgr_req_report_fault_t *req, uint64_t current_time);

// For testing
const fault_record_t* faultmgr_get_record(uint32_t domain_id);
void faultmgr_clear_state(void);

#endif // FAULTMGR_STATE_H
